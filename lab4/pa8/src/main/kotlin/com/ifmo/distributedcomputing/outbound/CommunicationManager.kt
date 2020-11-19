package com.ifmo.distributedcomputing.outbound

import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import mu.KLogging
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CopyOnWriteArraySet
import java.util.concurrent.CountDownLatch

class CommunicationManager(
  private val totalProcesses: Int,
  private val parentPort: Int,
  private val myId: Int,
  private val reactor: Reactor,
  private val startedLatch: CountDownLatch,
  private val doneLatch: CountDownLatch
) {

  private val connectors = ConcurrentHashMap<Int, Connector>()

  private val startedKek = CopyOnWriteArraySet<Int>()

  fun onMessageReceived(message: Message) {
    logger.info { "Received $message" }
    when (message.type) {
      MessageType.STARTED -> {
        startedLatch.countDown()
        startedKek.add(message.from)
      }
      MessageType.DONE -> doneLatch.countDown()
      else -> {
        logger.warn { "unable to handle msg $message" }
      }
    }
  }

  fun initiateConnections() {
    (0..totalProcesses).forEach {
      if (it == myId) return@forEach
      val targetProcess = it
      val targetPort = parentPort + it
      var localPort = parentPort * 2 + myId

      var success = false
      while (!success) {
        try { //TODO: timeouts here, seems that it doesnt work on 50+ processes
          logger.info { "Trying to connect to localhost:${targetPort} from local port $localPort" }
          val connector = Connector(
              "localhost",
              targetPort,
              targetProcess,
              localPort)
          connector.connect()
          reactor.registerHandler(ReactorEventType.WRITE, connector)
          success = true
          connectors[it] = connector
        } catch (e: Exception) {
          logger.warn { "Error сonnecting to $targetPort from $localPort: $e" }
          reactor.eventLoop(1000)
        }
      }
    }
  }

  fun broadcastStarted() {
    broadcast(Message(myId, -1, MessageType.STARTED))
  }

  fun awaitEveryoneStarted() {
    while (startedLatch.count > 1) {
      reactor.eventLoop(1000)
    }
  }

  fun broadcastDone() {
    broadcast(Message(myId, -1, MessageType.DONE))
  }

  fun awaitEveryoneDone() {
    while (doneLatch.count > 1) {
      reactor.eventLoop(1000)
    }
  }

  fun send(message: Message) {
    connectors[message.to]?.send(message) ?: error("no connector for ${message.to}")
  }

  fun broadcast(message: Message) {
    connectors.forEach { (_, connector) ->
      reactor.eventLoop(100)
      connector.send(message)
    }
  }

  fun close() {
    connectors.forEach { (_, connector) ->
      connector.close()
    }
  }

  companion object : KLogging()
}