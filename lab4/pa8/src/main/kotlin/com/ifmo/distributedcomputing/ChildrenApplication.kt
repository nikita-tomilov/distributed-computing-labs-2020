package com.ifmo.distributedcomputing

import com.ifmo.distributedcomputing.inbound.Acceptor
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import com.ifmo.distributedcomputing.logic.LamportMutex
import com.ifmo.distributedcomputing.logic.LamportTime
import com.ifmo.distributedcomputing.outbound.CommunicationManager
import mu.KLogging
import java.util.concurrent.CountDownLatch

object ChildrenApplication : KLogging() {

  fun child(parentPort: Int, myId: Int, totalProcesses: Int, usingMutex: Boolean) {
    Thread.currentThread().name = "child-$myId"

    val startedLatch = CountDownLatch(totalProcesses)
    val doneLatch = CountDownLatch(totalProcesses)

    val time = LamportTime()

    val reactor = Reactor()
    val cm = CommunicationManager(
        totalProcesses,
        parentPort,
        myId,
        reactor,
        startedLatch,
        doneLatch,
        time)
    val acceptor = Acceptor(parentPort + myId, reactor) { cm.onMessageReceived(it) }
    acceptor.setup()
    reactor.registerHandler(ReactorEventType.ACCEPT, acceptor)

    val mutex = LamportMutex(cm, reactor, myId, doneLatch, time)

    cm.initiateConnections()

    cm.broadcastStarted()
    logger.warn { "STARTED" }
    cm.awaitEveryoneStarted()
    logger.warn { "Received all STARTED messages" }

    val total = myId * 5
    (1..total).forEach {
      if (usingMutex) mutex.requestCS()
      logger.warn { "Process $myId doing iteration $it out of $total" }
      if (usingMutex) mutex.releaseCS()
    }

    cm.broadcastDone()
    logger.warn { "DONE" }
    cm.awaitEveryoneDone()
    logger.warn { "Received all DONE messages" }

    reactor.eventLoop(1000)
    acceptor.close()
    cm.close()
    reactor.closeAll()
  }
}