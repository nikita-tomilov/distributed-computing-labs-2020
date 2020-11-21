package com.ifmo.distributedcomputing.outbound

import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import com.ifmo.distributedcomputing.logic.LamportTime
import mu.KLogging
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CountDownLatch

class CommunicationManager(
  private val totalProcesses: Int,
  private val parentPort: Int,
  private val myId: Int,
  private val reactor: Reactor,
  private val startedLatch: CountDownLatch,
  private val doneLatch: CountDownLatch,
  private val time: LamportTime
) {

  private val connectors = ConcurrentHashMap<Int, Connector>()

  var csMessageHandler: (Message) -> Unit = {}

  fun onMessageReceived(message: Message) {
    logger.info { "Received $message" }
    time.updateAndIncrement(message.time)
    when (message.type) {
      MessageType.STARTED -> startedLatch.countDown()
      MessageType.DONE -> doneLatch.countDown()
      else -> csMessageHandler.invoke(message)
    }
  }

  fun initiateConnections() {
    (0..totalProcesses).forEach {
      if (it == myId) return@forEach
      val targetProcess = it
      val targetPort = parentPort + it
      val localPort = parentPort * 2 + myId //TODO: if bind failed, try increase local port?

      var success = false
      while (!success) {
        try {
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
          logger.warn { "Error connecting to $targetPort from $localPort: $e" }
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
    message.time = time.incrementAndGet()
    connectors[message.to]?.send(message) ?: error("no connector for ${message.to}")
  }

  fun broadcast(message: Message) {
    message.time = time.incrementAndGet()
    (0..totalProcesses).forEach {
      val connector = connectors[it] ?: return@forEach
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