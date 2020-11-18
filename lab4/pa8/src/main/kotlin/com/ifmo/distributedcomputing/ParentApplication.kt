package com.ifmo.distributedcomputing

import com.ifmo.distributedcomputing.inbound.Acceptor
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import mu.KLogging
import java.lang.reflect.Field

object ParentApplication : KLogging() {

  fun parent(N: Int) {
    Thread.currentThread().name = "parent"
    logger.info { "Entered Parent" }
    val port = 33000

    val reactor = Reactor()
    val acceptor = Acceptor(port, reactor)
    acceptor.setup()
    reactor.registerHandler(ReactorEventType.ACCEPT, acceptor)
    spawnChilds(N, port)

    while (true) {
      reactor.eventLoop(1000)
      logger.warn { "reactor loop in parent" }
    }
  }

  private fun spawnChilds(N: Int, port: Int) {
    (1..N).forEach {
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