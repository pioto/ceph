// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#include "msg/Message.h"
#include "DispatchQueue.h"
#include "SimpleMessenger.h"
#include "common/ceph_context.h"

#define dout_subsys ceph_subsys_ms
#include "common/debug.h"


/*******************
 * DispatchQueue
 */

#undef dout_prefix
#define dout_prefix *_dout << "-- " << msgr->get_myaddr() << " "

void DispatchQueue::enqueue(Message *m, int priority, uint64_t id)
{
  Mutex::Locker l(lock);
  ldout(cct,20) << "queue " << m << " prio " << priority << dendl;
  if (priority >= CEPH_MSG_PRIO_LOW) {
    mqueue.enqueue_strict(
      id, priority, QueueItem(m));
  } else {
    mqueue.enqueue(
      id, priority, m->cost(), QueueItem(m));
  }
  cond.Signal();
}

void DispatchQueue::local_delivery(Message *m, int priority)
{
  Mutex::Locker l(lock);
  m->set_connection(msgr->local_connection->get());
  if (priority >= CEPH_MSG_PRIO_LOW) {
    mqueue.enqueue_strict(
      0, priority, QueueItem(m));
  } else {
    mqueue.enqueue(
      0, priority, m->cost(), QueueItem(m));
  }
  cond.Signal();
}

/*
 * This function delivers incoming messages to the Messenger.
 * Pipes with messages are kept in queues; when beginning a message
 * delivery the highest-priority queue is selected, the pipe from the
 * front of the queue is removed, and its message read. If the pipe
 * has remaining messages at that priority level, it is re-placed on to the
 * end of the queue. If the queue is empty; it's removed.
 * The message is then delivered and the process starts again.
 */
void DispatchQueue::entry()
{
  lock.Lock();
  while (!stop) {
    while (!mqueue.empty() && !stop) {
      QueueItem qitem = mqueue.dequeue();
      lock.Unlock();

      if (qitem.is_code()) {
	switch (qitem.get_code()) {
	case D_BAD_REMOTE_RESET:
	  msgr->ms_deliver_handle_remote_reset(qitem.get_connection());
	  break;
	case D_CONNECT:
	  msgr->ms_deliver_handle_connect(qitem.get_connection());
	  break;
	case D_ACCEPT:
	  msgr->ms_deliver_handle_accept(qitem.get_connection());
	  break;
	case D_BAD_RESET:
	  msgr->ms_deliver_handle_reset(qitem.get_connection());
	  break;
	default:
	  assert(0);
	}
      } else {
	Message *m = qitem.get_message();
	uint64_t msize = m->get_dispatch_throttle_size();
	m->set_dispatch_throttle_size(0);  // clear it out, in case we requeue this message.

	ldout(cct,1) << "<== " << m->get_source_inst()
		     << " " << m->get_seq()
		     << " ==== " << *m
		     << " ==== " << m->get_payload().length() << "+" << m->get_middle().length()
		     << "+" << m->get_data().length()
		     << " (" << m->get_footer().front_crc << " " << m->get_footer().middle_crc
		     << " " << m->get_footer().data_crc << ")"
		     << " " << m << " con " << m->get_connection()
		     << dendl;
	msgr->ms_deliver_dispatch(m);

	msgr->dispatch_throttle_release(msize);

	ldout(cct,20) << "done calling dispatch on " << m << dendl;
      }

      lock.Lock();
    }
    if (!stop)
      cond.Wait(lock); //wait for something to be put on queue
  }
  lock.Unlock();
}

void DispatchQueue::discard_queue(uint64_t id) {
  Mutex::Locker l(lock);
  list<QueueItem> removed;
  mqueue.remove_by_class(id, &removed);
  for (list<QueueItem>::iterator i = removed.begin();
       i != removed.end();
       ++i) {
    assert(!(i->is_code())); // We don't discard id 0, ever!
    Message *m = i->get_message();
    msgr->dispatch_throttle_release(m->get_dispatch_throttle_size());
    m->put();
  }
}

void DispatchQueue::start()
{
  assert(!stop);
  assert(!dispatch_thread.is_started());
  dispatch_thread.create();
}

void DispatchQueue::wait()
{
  dispatch_thread.join();
}

void DispatchQueue::shutdown()
{
  // stop my dispatch thread
  lock.Lock();
  stop = true;
  cond.Signal();
  lock.Unlock();
}
