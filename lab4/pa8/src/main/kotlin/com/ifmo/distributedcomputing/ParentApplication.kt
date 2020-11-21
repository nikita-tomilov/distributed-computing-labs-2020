package com.ifmo.distributedcomputing

import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.inbound.Acceptor
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import mu.KLogging
import java.lang.reflect.Field
import java.util.concurrent.CountDownLatch

object ParentApplication : KLogging() {

  fun parent(N: Int) {
    Thread.currentThread().name = "parent"
    logger.info { "Entered Parent" }
    val port = 15000

    val reactor = Reactor()
    val doneLatch = CountDownLatch(N)

    val acceptor = Acceptor(port, reactor) {
      logger.info { "Parent accepted message $it" }
      if (it.type == MessageType.DONE) {
        doneLatch.countDown()
      }
    }
    acceptor.setup()
    reactor.registerHandler(ReactorEventType.ACCEPT, acceptor)
    val children = spawnChilds(N, port)

    while (doneLatch.count > 0) {
      reactor.eventLoop(1000)
    }
    reactor.eventLoop(1000)

    logger.warn { "Everyone reported to be finished; awaiting for PIDs to stop" }
    children.forEach { it.waitFor() }
    logger.warn { "Done" }
    acceptor.close()
    reactor.closeAll()
  }

  private fun spawnChilds(N: Int, port: Int): List<Process> {
    return (1..N).map {
      val cp = System.getProperty("java.class.path")
      val pb = ProcessBuilder(
          "java",
          "-cp",
          cp,
          "com.ifmo.distributedcomputing.Application",
          "--forked",
          "$port",
          "$it",
          "$N")
          .inheritIO()
      val process = pb.start()
      logger.warn { "Started child process ${process.getPid()}" }
      process
    }
  }

  private fun Process.getPid(): Long {
    var pid: Long = -1

    try {
      if (this.javaClass.name == "java.lang.UNIXProcess") {
        val f: Field = this.javaClass.getDeclaredField("pid")
        f.isAccessible = true
        pid = f.getLong(this)
        f.isAccessible = false
      }
    } catch (e: Exception) {
      pid = -1
    }
    return pid
  }
}