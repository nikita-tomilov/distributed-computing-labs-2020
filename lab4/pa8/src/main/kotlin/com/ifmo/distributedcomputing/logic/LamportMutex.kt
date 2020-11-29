package com.ifmo.distributedcomputing.logic

import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.outbound.CommunicationManager
import com.sun.jmx.remote.internal.ArrayQueue
import mu.KLogging
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicLong

class LamportMutex(
  private val communicationManager: CommunicationManager,
  private val reactor: Reactor,
  private val myId: Int,
  private val totalProcesses: CountDownLatch,
  private val time: LamportTime
) {

  private val queue = ArrayList<LamportMutexQueueEntry>()

  private val acknowledgements = AtomicLong(1)

  private val waitCounter = AtomicLong(0)

  init {
    communicationManager.csMessageHandler = {
      when (it.type) {
        MessageType.CS_REQUEST -> handleCSRequest(it)
        MessageType.CS_REPLY -> handleCSReply(it)
        MessageType.CS_RELEASE -> handleCSRelease(it)
        else -> {
        }
      }
    }
  }

  fun requestCS() {
    sendCSRequest()
    waitForReplies()
    waitForQueue()
  }

  fun releaseCS() {
    sendCSRelease()
  }

  private fun sendCSRequest() {
    acknowledgements.set(1)
    val m = Message(myId, -1, MessageType.CS_REQUEST)
    communicationManager.broadcast(m)
    addToQueue(LamportMutexQueueEntry(time.get(), myId))
  }

  private fun waitForReplies() {
    while (acknowledgements.get() < totalProcesses.count) {
      reactor.eventLoop(100)
      logger.info { "waiting for replies, got ${acknowledgements.get()} out of ${totalProcesses.count}" }
      waitCounter.incrementAndGet()
      if (waitCounter.get() % 100L == 0L) {
        logger.warn { "STUCK in waiting for replies, got ${acknowledgements.get()} out of ${totalProcesses.count}" }
      }
    }
    waitCounter.set(0)
  }

  private fun waitForQueue() {
    var condition: Boolean
    synchronized(queue) {
      condition = queue[0].id != myId
    }
    while (condition) {
      reactor.eventLoop(100)
      logger.info { "waiting for queue, it is $queue" }
      waitCounter.incrementAndGet()
      if (waitCounter.get() % 100L == 0L) {
        logger.warn { "STUCK in waiting for queue, it is $queue" }
        synchronized(queue) {
          queueSanityCheck()
        }
      }
      synchronized(queue) {
        condition = queue[0].id != myId
      }
    }
    waitCounter.set(0)
  }

  private fun sendCSRelease() {
    val m = Message(myId, -1, MessageType.CS_RELEASE)
    communicationManager.broadcast(m)
    deleteFromQueue(myId)
  }

  private fun handleCSRequest(m: Message) {
    val newQueueEntry = LamportMutexQueueEntry(m.time, m.from)
    addToQueue(newQueueEntry)
    communicationManager.send(Message(myId, m.from, MessageType.CS_REPLY))
  }

  private fun handleCSReply(m: Message) {
    this.acknowledgements.incrementAndGet()
    logger.info { "${m.from} acknowledged reply, it is now ${this.acknowledgements.get()}" }
  }

  private fun handleCSRelease(m: Message) {
    deleteFromQueue(m.from)
  }

  private fun sortQueue() {
    queue.sortWith(compareBy({ it.time }, { it.id }))
  }

  private fun addToQueue(entry: LamportMutexQueueEntry) {
    synchronized(queue) {
      queue.add(entry)
      sortQueue()
      queueSanityCheck()
    }
  }

  private fun deleteFromQueue(id: Int) {
    synchronized(queue) {
      queue.removeIf { it.id == id }
      sortQueue()
    }
  }

  private fun queueSanityCheck() {
    val ids = HashSet<Int>()
    val duplicates = HashSet<Int>()
    queue.forEach {
      if (ids.contains(it.id)) {
        logger.warn("Found duplicate queue entry for child id ${it.id}: $it")
        duplicates.add(it.id)
      } else ids.add(it.id)
    }
    duplicates.forEach { id ->
      val lastEntry = queue.lastOrNull { it.id == id }
      if (lastEntry != null) {
        queue.removeIf { (it != lastEntry) && (it.id == id) }
      }
    }
  }

  companion object : KLogging()
}

data class LamportMutexQueueEntry(
  val time: Long,
  val id: Int
)