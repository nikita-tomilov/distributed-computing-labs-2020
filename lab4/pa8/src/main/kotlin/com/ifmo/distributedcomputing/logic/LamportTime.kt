package com.ifmo.distributedcomputing.logic

class LamportTime {

  private var time = 1L

  @Synchronized
  fun get() = time

  @Synchronized
  fun incrementAndGet(): Long {
    time += 1
    return time
  }

  @Synchronized
  fun updateAndIncrement(another: Long): Long {
    val current = time
    if (current > another) {
      time = another
    }
    time += 1
    return time
  }
}