package com.ifmo.distributedcomputing.logic

import java.util.concurrent.atomic.AtomicLong

class LamportTime {

  private val time = AtomicLong(0L)

  fun get() = time.get()

  private fun set(time: Long) {
    this.time.set(time)
  }

  fun update(another: Long): Long {
    val current = get()
    if (current > another) {
      set(another)
    }
    return get()
  }

  fun updateAndIncrement(another: Long): Long {
    update(another)
    time.incrementAndGet()
    return get()
  }
}