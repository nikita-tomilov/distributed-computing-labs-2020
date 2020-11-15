package com.ifmo.distributedcomputing

data class Message(
  val from: Int,
  val type: MessageType
)

enum class MessageType {
  STARTED, DONE, CS_REQUEST, CS_REPLY, CS_RELEASE
}