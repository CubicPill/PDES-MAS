/*
 * Alp.cpp
 *
 *  Created on: 9 Sep 2010
 *      Author: Dr B.G.W. Craenen <b.craenen@cs.bham.ac.uk>
 */
#include <cassert>
#include "Alp.h"
#include "Agent.h"
#include "Log.h"
#include "Initialisor.h"

#ifdef TIMER
#include "Helper.h"
#include "FilePrint.h"
#endif

#include "SingleReadAntiMessage.h"
#include "WriteAntiMessage.h"

Alp::Alp(unsigned int pRank, unsigned int pCommSize,
         unsigned int pNumberOfClps, unsigned int pNumberOfAlps,
         unsigned long pStartTime, unsigned long pEndTime,
         const Initialisor *initialisor) {
  SetRank(pRank);
  SetSize(pCommSize);
  SetNumberOfClps(pNumberOfClps);
  SetNumberOfAlps(pNumberOfAlps);
  fStartTime = pStartTime;
  fEndTime = pEndTime;
  SetGvt(fStartTime);

  message_id_handler_ = new IdentifierHandler(pRank, pNumberOfClps, pNumberOfAlps);

  fGVTCalculator = new GvtCalculator(this);
  fGVT = fStartTime;


  fProcessMessageMutex = Mutex(ERRORCHECK);


  auto iter = initialisor->GetAlpToClpMap().find(GetRank());
  if (iter != initialisor->GetAlpToClpMap().end()) {
    fParentClp = iter->second;
  } else {
    LOG(logERROR)
      << "Alp::Alp# Couldn't initialise parent rank from initialisation file!";
    exit(1);
  }

  fMPIInterface = new MpiInterface(this, this);

  MPI_Barrier(MPI_COMM_WORLD);

  fMPIInterface->Signal();

  MPI_Barrier(MPI_COMM_WORLD);
}

Semaphore &Alp::GetWaitingSemaphore(unsigned long agent_id) {
  auto old_sem = agent_waiting_semaphore_map_.find(agent_id);
  if (old_sem != agent_waiting_semaphore_map_.end()) {
    // cleanup old
    // deleting nullptr is ok
    delete old_sem->second;
  }
  auto new_semaphore = new Semaphore();
  agent_waiting_semaphore_map_[agent_id] = new_semaphore;
  return *new_semaphore;
}

const AbstractMessage *Alp::GetResponseMessage(unsigned long agent_id) const {
  assert(agent_response_map_.find(agent_id) != agent_response_map_.end());
  return agent_response_map_.find(agent_id)->second;
}

bool Alp::AddAgent(unsigned long agent_id, Agent *agent) {
  if (managed_agents_.find(agent_id) != managed_agents_.end()) {
    return false;// already in list
  }
  managed_agents_[agent_id] = agent;
  return true;
}

void Alp::Send() {
  AbstractMessage *message;
  if (!fSendControlMessageQueue->IsEmpty()) {
    message = fSendControlMessageQueue->DequeueMessage();
  } else if (!fSendMessageQueue->IsEmpty()) {
    message = fSendMessageQueue->DequeueMessage();
    switch (message->GetType()) {
      case SINGLEREADMESSAGE: {
        SingleReadMessage *singleReadMessage = static_cast<SingleReadMessage *>(message);
        AddToSendList(singleReadMessage);
        PreProcessSendMessage(singleReadMessage);
      }
        break;
      case SINGLEREADRESPONSEMESSAGE :
        PreProcessSendMessage(dynamic_cast<SingleReadResponseMessage *> (message));
        break;
      case SINGLEREADANTIMESSAGE :
        PreProcessSendMessage(static_cast<SingleReadAntiMessage *> (message));
        break;
      case WRITEMESSAGE : {
        WriteMessage *writeMessage = static_cast<WriteMessage *>(message);
        AddToSendList(writeMessage);
        PreProcessSendMessage(writeMessage);
      }
        break;
      case WRITERESPONSEMESSAGE :
        PreProcessSendMessage(static_cast<WriteResponseMessage *> (message));
        break;
      case WRITEANTIMESSAGE :
        PreProcessSendMessage(static_cast<WriteAntiMessage *> (message));
        break;
      case RANGEQUERYMESSAGE: {
        RangeQueryMessage *rangeQueryMessage = static_cast<RangeQueryMessage *>(message);
        AddToSendList(rangeQueryMessage);
        PreProcessSendMessage(rangeQueryMessage);
      }
        break;
      case RANGEQUERYANTIMESSAGE :
        PreProcessSendMessage(static_cast<RangeQueryAntiMessage *> (message));
        break;
      case ROLLBACKMESSAGE :
        PreProcessSendMessage(static_cast<RollbackMessage *> (message));
        break;
      default: // skip
        break;
    }
  } else {
    // A message signal was issues, but no message remains to be send, most probably because
    // the message was removed because of a rollback.
    return;
  }
  fMPIInterface->Send(message);
}

void Alp::Receive() {
  // Fetch received message from the receive queue
  AbstractMessage *message = fReceiveMessageQueue->DequeueMessage();
  fProcessMessageMutex.Lock();
  switch (message->GetType()) {
    case SINGLEREADRESPONSEMESSAGE: {
      SingleReadResponseMessage *singleReadResponseMessage = dynamic_cast<SingleReadResponseMessage *>(message);
      PreProcessReceiveMessage(singleReadResponseMessage);
      ProcessMessage(singleReadResponseMessage);
    }
      break;
    case WRITERESPONSEMESSAGE: {
      WriteResponseMessage *writeResponseMessage = dynamic_cast<WriteResponseMessage *>(message);
      PreProcessReceiveMessage(writeResponseMessage);
      ProcessMessage(writeResponseMessage);
    }
      break;
    case RANGEQUERYMESSAGE: {
      RangeQueryMessage *rangeQueryMessage = dynamic_cast<RangeQueryMessage *>(message);
      PreProcessReceiveMessage(rangeQueryMessage);
      ProcessMessage(rangeQueryMessage);
    }
      break;
    case ROLLBACKMESSAGE: {
      RollbackMessage *rollbackMessage = dynamic_cast<RollbackMessage *>(message);
      PreProcessReceiveMessage(rollbackMessage);
      ProcessMessage(rollbackMessage);
    }
      break;
    case GVTCONTROLMESSAGE:
      fGVTCalculator->ProcessMessage(
          dynamic_cast<const GvtControlMessage *> (message));
      break;
    case GVTREQUESTMESSAGE:
      fGVTCalculator->ProcessMessage(
          dynamic_cast<const GvtRequestMessage *> (message));
      break;
    case GVTVALUEMESSAGE:
      fGVTCalculator->ProcessMessage(
          dynamic_cast<const GvtValueMessage *> (message));
      break;
    default:
      LOG(logERROR)
        << "Alp::Receive(" << GetRank()
        << ")# Received inappropriate message: " << *message;
      exit(1);
  }
  fProcessMessageMutex.Unlock();
  /*
   * Don't free the memory for value here! That memory _address_ is copied for the interface,
   * and the value is still needed there. Value memory will be freed there.
   */
  delete message;
}

void Alp::ProcessMessage(const RollbackMessage *pRollbackMessage) {


  ProcessRollback(pRollbackMessage);

}

void Alp::ProcessMessage(const SingleReadResponseMessage *pSingleReadResponseMessage) {
  unsigned long message_id = pSingleReadResponseMessage->GetIdentifier();
  agent_response_map_[message_id] = pSingleReadResponseMessage;
}

void Alp::ProcessMessage(const WriteResponseMessage *pWriteResponseMessage) {
  unsigned long message_id = pWriteResponseMessage->GetIdentifier();
  agent_response_map_[message_id] = pWriteResponseMessage;
}

void Alp::ProcessMessage(const RangeQueryMessage *pRangeQueryMessage) {
  unsigned long message_id = pRangeQueryMessage->GetIdentifier();
  agent_response_map_[message_id] = pRangeQueryMessage;
}

/*When rollback message arrives, agent can be in the waiting state or not in waiting state
 * If not waiting, just rollback the statebase and LVT
 * If waiting, need to signal that semaphore, and perform statebase rollback and LVT rollback,
 * and discard incoming invalidated message (if any)
 * Also, the agent thread needs to be suspended
 */
bool Alp::ProcessRollback(const RollbackMessage *pRollbackMessage) {
  // commonly used


  unsigned long rollback_message_timestamp = pRollbackMessage->GetTimestamp();
  unsigned long agent_id = pRollbackMessage->GetOriginalAgent().GetId();
  Agent *agent = managed_agents_[agent_id];
  // make sure agent under this ALP
  assert(HasAgent(agent_id));

  // Check if rollback already on tag list
  if (CheckRollbackTagList(pRollbackMessage->GetRollbackTag())) {
    LOG(logFINEST)
      << "Alp::ProcessRollback(" << GetRank()
      << ")# Rollback message tag already on list, ignore rollback: "
      << *pRollbackMessage;
    return false;
  }
  // Check if rollback rolls back far enough
  if (pRollbackMessage->GetTimestamp() > GetAgentLvt(pRollbackMessage->GetOriginalAgent().GetId())) {
    return false;
  }
  // Rollback message is good, rollback

  // Reset LVT map
  LOG(logFINEST) << "Alp::ProcessRollback(" << GetRank() << ")# Rollback agent : "
                 << pRollbackMessage->GetOriginalAgent().GetId() << ", to LVT: " << pRollbackMessage->GetTimestamp();


  if (rollback_message_timestamp < 0) {
    LOG(logERROR) << "HasIDLVTMap::RollbackAgentLVT# Rollback time smaller then 0, agent: " << agent_id << ", LVT: "
                  << agent_lvt_map_[agent_id] << ", rollback time: " << rollback_message_timestamp;
    exit(1);
  }
  if (rollback_message_timestamp >= agent_lvt_map_[agent_id]) {
    LOG(logERROR) << "HasIDLVTMap::RollbackAgentLVT# Rollback time not smaller then LVT, agent: " << agent_id
                  << ", LVT: " << agent_lvt_map_[agent_id] << ", rollback time: " << rollback_message_timestamp;
    exit(1);
  }
  // stop the agent
  agent->Stop();

  // reset semaphore
  delete agent_waiting_semaphore_map_[agent_id];
  agent_waiting_semaphore_map_[agent_id] = nullptr;

  // rollback LVT and history
  unsigned long rollback_to_timestamp = ULONG_MAX;
  auto &agent_lvt_history = agent_lvt_history_map_[agent_id];
  for (auto iter:agent_lvt_history) {
    if (iter < rollback_message_timestamp) {
      rollback_message_timestamp = iter;
    } else {
      break;
    }
  }

  agent_lvt_map_[agent_id] = rollback_to_timestamp;
  agent_lvt_history.erase(
      remove_if(
          agent_lvt_history.begin(),
          agent_lvt_history.end(),
          [rollback_to_timestamp](unsigned long t) { return t > rollback_to_timestamp; }
      ),
      agent_lvt_history.end()
  );

  /*
   We first need to remove all those events with time stamp greater than
   that of the roll-back which have been generated and not yet sent.  i.e.,
   remove all messages in the sendQ which have time greater than roll-back
   time. We do this before generating the anti-messages to prevent them from
   also being deleted from the sendQ.
   */
  fSendMessageQueue->RemoveMessages(pRollbackMessage->GetOriginalAgent(), pRollbackMessage->GetTimestamp());
  /*
   Next stage of roll-back is to process the sentList, sending out
   anti-messages for all the messages that have been sent out by this
   Alp after the time of the roll-back.
   */
  list<SharedStateMessage *> rollbackMessagesInSendList = RollbackSendList(pRollbackMessage->GetTimestamp(),
                                                                           pRollbackMessage->GetOriginalAgent());
  for (list<SharedStateMessage *>::iterator iter = rollbackMessagesInSendList.begin();
       iter != rollbackMessagesInSendList.end();) {
    if (fGVT > (*iter)->GetTimestamp()) {
      LOG(logWARNING)
        << "Alp::ProcessRollback(" << GetRank()
        << ")# Rolling back before gvt: " << fGVT << " Message Time: "
        << (*iter)->GetTimestamp() << " Message: " << *iter;
    } else {
      switch ((*iter)->GetType()) {
        case SINGLEREADMESSAGE : {
          SingleReadMessage *singleReadMessage = static_cast<SingleReadMessage *> (*iter);
          SingleReadAntiMessage *antiSingleReadMessage = new SingleReadAntiMessage();
          antiSingleReadMessage->SetOrigin(GetRank());
          antiSingleReadMessage->SetDestination(GetParentClp());
          antiSingleReadMessage->SetTimestamp(singleReadMessage->GetTimestamp());
          // Mattern colour?
          antiSingleReadMessage->SetNumberOfHops(0);
          antiSingleReadMessage->SetRollbackTag(pRollbackMessage->GetRollbackTag());
          antiSingleReadMessage->SetOriginalAgent(singleReadMessage->GetOriginalAgent());
          antiSingleReadMessage->SetSsvId(singleReadMessage->GetSsvId());
          antiSingleReadMessage->SendToLp(this);
        }
          break;
        case WRITEMESSAGE : {
          WriteMessage *writeMessage = static_cast<WriteMessage *> (*iter);
          WriteAntiMessage *antiWriteMessage = new WriteAntiMessage();
          antiWriteMessage->SetOrigin(GetRank());
          antiWriteMessage->SetDestination(GetParentClp());
          antiWriteMessage->SetTimestamp(writeMessage->GetTimestamp());
          // Mattern colour?
          antiWriteMessage->SetNumberOfHops(0);
          antiWriteMessage->SetRollbackTag(pRollbackMessage->GetRollbackTag());
          antiWriteMessage->SetOriginalAgent(writeMessage->GetOriginalAgent());
          antiWriteMessage->SetSsvId(writeMessage->GetSsvId());
          antiWriteMessage->SendToLp(this);
        }
          break;
        case RANGEQUERYMESSAGE : {
          RangeQueryMessage *rangeQueryMessage = static_cast<RangeQueryMessage *> (*iter);
          RangeQueryAntiMessage *antiRangeQueryMessage = new RangeQueryAntiMessage();
          antiRangeQueryMessage->SetOrigin(GetRank());
          antiRangeQueryMessage->SetDestination(GetParentClp());
          antiRangeQueryMessage->SetTimestamp(rangeQueryMessage->GetTimestamp());
          // Mattern colour?
          antiRangeQueryMessage->SetNumberOfHops(0);
          antiRangeQueryMessage->SetRollbackTag(pRollbackMessage->GetRollbackTag());
          antiRangeQueryMessage->SetOriginalAgent(rangeQueryMessage->GetOriginalAgent());
          antiRangeQueryMessage->SetRange(rangeQueryMessage->GetRange());
          antiRangeQueryMessage->SetIdentifier(rangeQueryMessage->GetIdentifier());
          antiRangeQueryMessage->SendToLp(this);
        }
          break;
        default :
          LOG(logWARNING)
            << "Alp::ProcessRollback(" << GetRank()
            << ")# Unsuitable message found to create antimessage from: "
            << *iter;
          break;
      }
    }
    // I don't need to free the memory for value for WriteMessages as that was freed by SendThread when the message
    // was send originally. It was never stored in the send-list anyway.
    delete *iter;
    rollbackMessagesInSendList.erase(iter++);
  }

  // restart agent
  agent->Start(nullptr);
  return true;
}

int Alp::GetParentClp() const {
  return fParentClp;
}

unsigned long Alp::GetLvt() const {
  unsigned long minimum_agent_lvt = ULONG_MAX;
  for (auto iter:agent_lvt_map_) {
    if (iter.second < minimum_agent_lvt) {
      minimum_agent_lvt = iter.second;
    }
  }
  return minimum_agent_lvt;
}

void Alp::SetGvt(unsigned long pGvt) {
  fGVT = pGvt;
  ClearSendList(pGvt);
  ClearRollbackTagList(pGvt);
}


unsigned long Alp::GetNewMessageId() const {
  return message_id_handler_->GetNextID();
}

bool Alp::HasAgent(unsigned long agent_id) {
  return agent_lvt_map_.find(agent_id) != agent_lvt_map_.end();
}

unsigned long Alp::GetAgentLvt(unsigned long agent_id) const {
  auto result = agent_lvt_map_.find(agent_id);
  if (result != agent_lvt_map_.end()) {
    return result->second;
  }
  return ULONG_MAX; //TODO this
}

bool Alp::SetAgentLvt(unsigned long agent_id, unsigned long lvt) {
  auto result = agent_lvt_map_.find(agent_id);
  if (result != agent_lvt_map_.end()) {
    if (result->second <= lvt) {
      result->second = lvt;
      return true;
    }
  }
  return false;
}

void Alp::Initialise() {
  // Nothing yet
}

void Alp::Finalise() {
  fMPIInterface->StopSimulation();
  fMPIInterface->Join();
}