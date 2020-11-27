package com.ifmo.distributedcomputing.ipc

import com.google.common.util.concurrent.ThreadFactoryBuilder
import mu.KLogging
import java.nio.channels.SelectionKey
import java.nio.channels.ServerSocketChannel
import java.nio.channels.SocketChannel
import java.util.concurrent.*
import kotlin.system.exitProcess

class Reactor {

  private val handlers = ConcurrentHashMap<ReactorEventType, CopyOnWriteArraySet<EventHandler>>()

  private val tp = Executors.newFixedThreadPool(4, ThreadFactoryBuilder().setDaemon(true).build())

  fun registerHandler(type: ReactorEventType, handler: EventHandler) {
    handlers.getOrPut(type) { CopyOnWriteArraySet() }.add(handler)
  }

  fun eventLoop(timeoutMs: Long) {
    val stopAt = System.currentTimeMillis() + timeoutMs
    while (System.currentTimeMillis() <= stopAt) {
      if (SelectorSingleton.selector.select(timeoutMs) == 0) {
        continue
      }
      val selectedKeys = SelectorSingleton.selector.selectedKeys().iterator()
      val queue = ArrayList<Runnable>()
      var latch = CountDownLatch(0)
      while (selectedKeys.hasNext()) {
        val selectedKey = selectedKeys.pop()
        when {
          selectedKey.isAcceptable -> handlers[ReactorEventType.ACCEPT]?.forEach {
            queue.add(Runnable {
              it.handle(selectedKey)
              latch.countDown()
            })
          }
          selectedKey.isReadable -> handlers[ReactorEventType.READ]?.forEach {
            queue.add(Runnable {
              it.handle(selectedKey)
              latch.countDown()
            })
          }
          selectedKey.isWritable -> handlers[ReactorEventType.WRITE]?.forEach {
            queue.add(Runnable {
              it.handle(selectedKey)
              latch.countDown()
            })
          }
        }
      }
      latch = CountDownLatch(queue.size)
      queue.forEach { tp.submit(it) }
      val ok = latch.await(10, TimeUnit.MINUTES)
      if (!ok) {
        logger.error { "latch await FAILED on latch size ${queue.size}" }
        exitProcess(-1)
      }
    }
  }

  fun closeAll() {
    SelectorSingleton.selector.keys().forEach {
      val c = it.channel()
      when (c) {
        is SocketChannel -> c.close()
        is ServerSocketChannel -> c.close()
      }
      c.close()
    }
  }

  companion object : KLogging()
}

private fun <T> MutableIterator<T>.pop(): T {
  val x = this.next()
  this.remove()
  return x
}

enum class ReactorEventType {
  ACCEPT, READ, WRITE
}

interface EventHandler {
  fun handle(selectionKey: SelectionKey)
}