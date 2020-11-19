package com.ifmo.distributedcomputing.outbound

import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import mu.KLogging
import java.util.concurrent.ConcurrentHashMap
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

  fun onMessageReceived(message: Message) {
    logger.info { "Received $message" }
    when (message.type) {
      MessageType.STARTED -> startedLatch.countDown()
      MessageType.DONE -> doneLatch.countDown()
      else -> {
        logger.warn { "unable to handle msg $message" }
      }
    }
  }

  fun initiateConnections() {
    (0..totalProcesses).forEach {
      val targetProcess = it
      val targetPort = parentPort + it

      var success = false
      while (!success) {
        try {
          val connector = Connector(
              "localhost",
              targetPort,
              targetProcess,
              0) //it - 10000 + myId * 100)
          connector.connect()
          reactor.registerHandler(ReactorEventType.WRITE, connector)
          success = true
          connectors[it] = connector
        } catch (e: Exception) {
          reactor.eventLoop(100)
        }
      }
    }
  }

  fun broadcastStarted() {
    broadcast(Message(myId, -1, MessageType.STARTED))
  }

  fun awaitEveryoneStarted() {
    while (startedLatch.count > 0) {
      reactor.eventLoop(1000)
    }
  }

  fun broadcastDone() {
    broadcast(Message(myId, -1, MessageType.DONE))
  }

  fun awaitEveryoneDone() {
    while (doneLatch.count > 0) {
      reactor.eventLoop(1000)
    }
  }

  fun send(message: Message) {
    connectors[message.to]?.send(message) ?: error("no connector for ${message.to}")
  }

  fun broadcast(message: Message) {
    connectors.forEach { (idx, connector) ->
      reactor.eventLoop(100)
      connector.send(message)
    }
  }

  companion object : KLogging()
}