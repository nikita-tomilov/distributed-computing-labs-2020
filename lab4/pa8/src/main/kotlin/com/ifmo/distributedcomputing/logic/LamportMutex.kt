package com.ifmo.distributedcomputing.logic

import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.dto.MessageType
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.outbound.CommunicationManager
import mu.KLogging
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicLong
import java.util.concurrent.atomic.AtomicReference

class LamportMutex(
  private val communicationManager: CommunicationManager,
  private val reactor: Reactor,
  private val myId: Int,
  private val totalProcesses: CountDownLatch,
  private val time: LamportTime
) {

  //TODO: HANDLE FUCKING TIME, IT GOES TO CYCLE
  // [LamportMutexQueueEntry(time=0, id=2), LamportMutexQueueEntry(time=0, id=1)]

  private val queue = AtomicReference<List<LamportMutexQueueEntry>>(emptyList())

  private val acknowledgements = AtomicLong(1)

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
      logger.info { "waitin for replies, got ${acknowledgements.get()} out of ${totalProcesses.count}" }
    }
  }

  private fun waitForQueue() {
    while (queue.get()[0].id != myId) {
      reactor.eventLoop(100)
      logger.info { "waitin for queue, it is ${queue.get()}" }
    }
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
    val sorted = queue.get().sortedWith(compareBy({ it.time }, { it.id }))
    queue.set(sorted)
  }

  private fun addToQueue(entry: LamportMutexQueueEntry) {
    val appended = queue.get() + entry
    queue.set(appended)
    sortQueue()
  }

  private fun deleteFromQueue(id: Int) {
    val updated = queue.get().filter { it.id != id }
    queue.set(updated)
    sortQueue()
  }

  companion object : KLogging()

}

data class LamportMutexQueueEntry(
  val time: Long,
  val id: Int
)