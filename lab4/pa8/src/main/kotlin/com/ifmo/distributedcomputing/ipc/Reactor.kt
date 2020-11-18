package com.ifmo.distributedcomputing.ipc

import mu.KLogging
import java.nio.channels.SelectionKey
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CopyOnWriteArraySet

class Reactor {

  private val handlers = ConcurrentHashMap<ReactorEventType, CopyOnWriteArraySet<EventHandler>>()

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
      while (selectedKeys.hasNext()) {
        val selectedKey = selectedKeys.pop()
        when {
          selectedKey.isAcceptable -> handlers[ReactorEventType.ACCEPT]?.forEach {
            it.handle(
                selectedKey)
          }
          selectedKey.isReadable -> handlers[ReactorEventType.READ]?.forEach { it.handle(selectedKey) }
          selectedKey.isWritable -> handlers[ReactorEventType.WRITE]?.forEach {
            it.handle(
                selectedKey)
          }
        }
      }
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