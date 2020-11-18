package com.ifmo.distributedcomputing

import com.ifmo.distributedcomputing.inbound.Acceptor
import mu.KLogging
import java.lang.reflect.Field
import java.util.concurrent.CountDownLatch

object ParentApplication : KLogging() {

  fun parent(N: Int) {
    Thread.currentThread().name = "parent"
    logger.info { "Entered Parent" }
    val port = 33000
    val latch = CountDownLatch(N)
    val acceptor = Acceptor(port, latch)
    acceptor.start()
    spawnChilds(
        N,
        port)
    latch.await()
    acceptor.stop()
    logger.warn { "Acceptor stopped" }
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