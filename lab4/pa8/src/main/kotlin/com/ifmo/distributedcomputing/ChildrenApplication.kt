package com.ifmo.distributedcomputing

import mu.KLogging
import java.util.concurrent.CountDownLatch

object ChildrenApplication : KLogging() {

  fun child(parentPort: Int, myId: Int, totalProcesses: Int) {
    Thread.currentThread().name = "child-$myId"
    logger.info { "Entered Child" }

    val latch = CountDownLatch(totalProcesses)
    val acceptor = Acceptor(parentPort + myId, latch)
    acceptor.start()

    Thread.sleep((totalProcesses * 100).toLong())

    val connectors = (parentPort..(parentPort + totalProcesses)).map {
      val connector = Connector("localhost", it, 0 )//it - 10000 + myId * 100)
      connector.connect()
      connector
    }

    logger.warn { "Connected to everyone" }

    connectors.forEach { connector ->
      connector.send(Message(myId, MessageType.STARTED))
    }
    connectors.forEach { connector ->
      connector.send(Message(myId, MessageType.DONE))
      connector.close()
    }
    latch.await()
    acceptor.stop()
  }
}