package com.ifmo.distributedcomputing

import com.ifmo.distributedcomputing.inbound.Acceptor
import com.ifmo.distributedcomputing.outbound.CommunicationManager
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import mu.KLogging
import java.util.concurrent.CountDownLatch

object ChildrenApplication : KLogging() {

  fun child(parentPort: Int, myId: Int, totalProcesses: Int) {
    Thread.currentThread().name = "child-$myId"
    logger.warn { "Entered Child" }

    val startedLatch = CountDownLatch(totalProcesses)
    val doneLatch = CountDownLatch(totalProcesses)

    val reactor = Reactor()
    val cm = CommunicationManager(
        totalProcesses,
        parentPort,
        myId,
        reactor,
        startedLatch,
        doneLatch)
    val acceptor = Acceptor(parentPort + myId, reactor) { cm.onMessageReceived(it) }
    acceptor.setup()
    reactor.registerHandler(ReactorEventType.ACCEPT, acceptor)

    cm.initiateConnections()
    logger.warn { "Connected to everyone" }

    cm.broadcastStarted()
    logger.warn { "Completed Started broadcast" }
    cm.awaitEveryoneStarted()
    logger.warn { "Other nodes Started" }

    cm.broadcastDone()
    logger.warn { "Completed Done broadcast" }
    cm.awaitEveryoneDone()
    logger.warn { "Other nodes Done" }

    reactor.eventLoop(1000)
    logger.warn { "Completed" }
    acceptor.close()
    cm.close()
    reactor.closeAll()
  }
}