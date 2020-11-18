package com.ifmo.distributedcomputing

import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.inbound.Acceptor
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import com.ifmo.distributedcomputing.outbound.Connector
import mu.KLogging
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CountDownLatch

object ChildrenApplication : KLogging() {

  fun child(parentPort: Int, myId: Int, totalProcesses: Int) {
    Thread.currentThread().name = "child-$myId"
    logger.info { "Entered Child" }

    val reactor = Reactor()
    val acceptor = Acceptor(parentPort + myId, reactor)
    acceptor.setup()
    reactor.registerHandler(ReactorEventType.ACCEPT, acceptor)

    val connectors = ConcurrentHashMap<Int, Connector>()
    val latch = CountDownLatch(1)

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

    logger.warn { "Connected to everyone" }

    connectors.forEach { (idx, connector) ->
      reactor.eventLoop(100)
      connector.send(
          Message(
              myId,
              idx,
              MessageType.STARTED))
    }
    connectors.forEach { (idx, connector) ->
      reactor.eventLoop(100)
      connector.send(
          Message(
              myId,
              idx,
              MessageType.DONE))
    }

    while (latch.count > 0) {
      reactor.eventLoop(1000)
    }

    logger.warn { "i finished" }
  }
}