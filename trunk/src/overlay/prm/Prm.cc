/*
 * Prm.cc
 *
 *  Created on: Jun 5, 2011
 *      Author: tproenca
 */

#include <omnetpp.h>
#include <time.h>
#include "Prm.h"

namespace oversim {

Define_Module(Prm);

MulticastMessageQueue::MulticastMessageQueue() : std::list<PrmMulticastMessage*>() {

}

void MulticastMessageQueue::push(PrmMulticastMessage* x) {
	if (size() >= BUFFER_SIZE) {
		PrmMulticastMessage* msg = front();
		pop();
		delete msg;
	}
	push_back(x);
}


Prm::Prm() :
	Nice(),last_pkt_id_(-1), duplicated_pkts_(0), received_pkts_(0), seqNo_(-1), naks_received_(0), naks_sent_(0),
	randomWalk_forwards_(0),randomWalk_initiated_(0), mesh_sent_pkts_(0) {
	// Initializing seed of the random number generator.
	srand(time(0));
//	srand(1000);
}

Prm::~Prm() {
	MulticastMessageQueue::iterator i;
	for(i = pkt_buffer_.begin(); i != pkt_buffer_.end(); i++) {
		delete *i;
	}
}

void Prm::handlePrmMulticast(PrmMulticastMessage* multicastMsg) {

	 if (multicastMsg->getSrcNode() != thisNode) {
		long seqNo = multicastMsg->getSeqNo();

		if (last_pkt_id_ != -1) { // se NAO eh a primeira vez

			if (seqNo > last_pkt_id_) { // New packet
				if (seqNo - last_pkt_id_ == 1) { // Without loss
					// Do nothing. Handled by Nice::handleNiceMulticast.
				} else { // With loss
					int pkts_missed = seqNo - last_pkt_id_ - 1;
					for (int i = last_pkt_id_ + 1; i <= (last_pkt_id_
							+ pkts_missed); i++) {
						missing_pkts_.push_back(i);
					}
				}
				last_pkt_id_ = seqNo;
				received_pkts_++;
				pkt_buffer_.push(multicastMsg->dup());
			} else { // Duplicated or Missing
				bool isMissing = find(missing_pkts_.begin(),
						missing_pkts_.end(), seqNo) != missing_pkts_.end();
				if (isMissing) { // is in the list, is missing
					received_pkts_++;
					missing_pkts_.remove(seqNo);
					pkt_buffer_.push(multicastMsg->dup());
				} else { // duplicated
					duplicated_pkts_++;
					delete multicastMsg;
					return;
				}

			}
		} else { // se eh a primeira vez
			last_pkt_id_ = seqNo;
			received_pkts_++;
			pkt_buffer_.push(multicastMsg->dup());
		}

		// prints the node status
		//printNodeStatus(multicastMsg);

		// Send a nak message to the last hop node asking for the missing packets.
		sendPrmNakMulticast(multicastMsg);

		// update message with node received list.
		updatePrmMulticast(multicastMsg);

		// Send message to the random childs.
		sendDataToChilds(multicastMsg);

		//sim_results_.record(missing_pkts_.size());
	}

	// Broadcast message to members of the cluster. Has to be called in the end
	// because it deletes the multicast message instance.
	handleNiceMulticast(multicastMsg);

	// Start random walk discovery for each DISCOVERY_INTERVAL packets.
//	if ((received_pkts_ % DISCOVERY_INTERVAL) == DISCOVERY_INTERVAL-1) {
//		startPrmRandomWalkDiscover();
//	}

	if ((rand() % (int)(1/0.001)) == 1){
		startPrmRandomWalkDiscover();
	}

}

void Prm::sendDataToChilds(PrmMulticastMessage* msg){
	if (msg->getSrcNode() != thisNode) {
		std::list<TransportAddress>::iterator it;
		for(it = childs_.begin(); it != childs_.end(); it++) {
			// Send message with to childs with % probability
			if ((rand() % (int)(1/BETA)) == 1) {
				PrmMulticastMessage* dup = msg->dup();
				dup->setLayer(-1);
				dup->setLastHop(thisNode);
				sendMessageToUDP(*it, dup);
				mesh_sent_pkts_++;
			}
		}
	}
	// Message is gonna be destroyed by handleNiceMulticast() method.
}

void Prm::printNodeStatus(PrmMulticastMessage* msg){
	std::cout << "--------" << "Node: " << thisNode << "--------"
			<< endl;
	std::cout << "last_pkt_id_:" << last_pkt_id_ << endl;
	std::cout << "msg_seqNo:" << msg->getSeqNo() << endl;
	std::cout << "received_pkts_:" << received_pkts_ << endl;
	std::cout << "duplicated_pkts_:" << duplicated_pkts_ << endl;

	std::cout << "Node buffer: " << endl;
	MulticastMessageQueue::iterator i;
	for (i = pkt_buffer_.begin(); i != pkt_buffer_.end(); i++) {
		std::cout << "pkt:" << (*i)->getSeqNo() << endl;
	}

	std::cout << "Parent buffer: " << msg->getLastHop() << endl;
	for(int j=0; j < msg->getPkts_seqNoArraySize(); j++){
		std::cout << "pkt:" << msg->getPkts_seqNo(j) << endl;
	}

	std::cout << "Node pkts missing: " << endl;
	std::list<int>::iterator k;
	for (k = missing_pkts_.begin(); k != missing_pkts_.end(); k++) {
		std::cout << "pkt:" << *k << endl;
	}
}

void Prm::updatePrmMulticast(PrmMulticastMessage* msg){
	msg->setPkts_seqNoArraySize(pkt_buffer_.size());
	int i = 0;
	MulticastMessageQueue::iterator j;
	for(j = pkt_buffer_.begin(); j != pkt_buffer_.end(); j++) {
		msg->setPkts_seqNo(i++, (*j)->getSeqNo());
	}
}

void Prm::sendPrmNakMulticast(PrmMulticastMessage* msg) {

	 if (msg->getSrcNode() != thisNode) {
		std::list<int> match_pkts;

		// Verifies if parent node has the missing packets
		std::list<int>::iterator it;
		for (it = missing_pkts_.begin(); it != missing_pkts_.end(); it++) {
			for (int i = 0; i < msg->getPkts_seqNoArraySize(); i++) {
				if (*it == msg->getPkts_seqNo(i)) {
					match_pkts.push_back(*it);
				}
			}
		}

		if (match_pkts.size() > 0) {
//			std::cout << "pai tem pacotes" << endl;
			PrmMulticastMessage *prmMsg = new PrmMulticastMessage(
					"PRM_NAK_MULTICAST");
			prmMsg->setCommand(PRM_NAK_MULTICAST);
			prmMsg->setLayer(-1);
			prmMsg->setSrcNode(thisNode);
			prmMsg->setLastHop(thisNode);
			prmMsg->setHopCount(0);
			prmMsg->setSeqNo(0);
			prmMsg->setBitLength(PRMMULTICAST_L(prmMsg));

			// Sets in the message the missing packets that the parent has.
			int i = 0;
			prmMsg->setPkts_seqNoArraySize(match_pkts.size());
			for (it = match_pkts.begin(); it != match_pkts.end(); it++) {
				prmMsg->setPkts_seqNo(i++, *it);
			}

			sendMessageToUDP(msg->getLastHop(), prmMsg);
			naks_sent_++;

			// Message is gonna be destroyed by handlePrmNakMulticast() method.
		}
	}
}

void Prm::handlePrmNakMulticast(PrmMulticastMessage* msg) {
	if (msg->getSrcNode() != thisNode) {
		for (int i = 0; i < msg->getPkts_seqNoArraySize(); i++) {
			long msgSeqNo = msg->getPkts_seqNo(i);

			MulticastMessageQueue::iterator it;
			for (it = pkt_buffer_.begin(); it != pkt_buffer_.end(); it++) {
				if ((*it)->getSeqNo() == msgSeqNo) {
					PrmMulticastMessage* dup =
							static_cast<PrmMulticastMessage*> ((*it)->dup());
					updatePrmMulticast(dup);
					dup->setLastHop(thisNode);
					sendMessageToUDP(msg->getSrcNode(), dup);
				}
			}

		}
		naks_received_++;
	}
	delete msg;
}

void Prm::handleAppMessage(cMessage* msg)
{
	if ( ALMMulticastMessage* multicastMsg = dynamic_cast<ALMMulticastMessage*>(msg) ) {
		PrmMulticastMessage *prmMsg = new PrmMulticastMessage("PRM_MULTICAST");
        prmMsg->setCommand(PRM_MULTICAST);
        prmMsg->setLayer(-1);
        prmMsg->setSrcNode(thisNode);
        prmMsg->setLastHop(thisNode);
        prmMsg->setHopCount(0);
        prmMsg->setSeqNo(++seqNo_);

        prmMsg->setBitLength(PRMMULTICAST_L(prmMsg));

        prmMsg->encapsulate(multicastMsg);
        sendDataToOverlay(prmMsg);

        // otherwise msg gets deleted later
        msg = NULL;
	} else {
		Nice::handleAppMessage(msg);
	}
} // handleAppMessage

void Prm::handleUDPMessage(BaseOverlayMessage* msg) {
	if (dynamic_cast<NiceMessage*>(msg) != NULL) {
		NiceMessage* niceMsg = check_and_cast<NiceMessage*>(msg);

        // First of all, update activity information for sourcenode
        std::map<TransportAddress, NicePeerInfo*>::iterator it = peerInfos.find(niceMsg->getSrcNode());

        if (it != peerInfos.end()) {
            it->second->touch();
        }

//        if (thisNode.getIp().str().compare("1.0.0.78") == 0) {
//    		std::cout << thisNode.getIp().str() << " recebe de " << niceMsg->getSrcNode() << " ("<<  niceMsg->getCommand() << ")" << endl;
//    	}

        /* Dispatch message, possibly downcasting to a more concrete type */
        switch (niceMsg->getCommand()) {

        PrmMulticastMessage *multicastMsg;
        PrmRandomWalkMessage *walkMsg;

        case NICE_MULTICAST:
        	std::cout << "Error: Should not have NICE_MULTICAST message in PRM simulation." << std::endl;
        	break;
        case PRM_MULTICAST:
        	multicastMsg = check_and_cast<PrmMulticastMessage*>(msg);
        	handlePrmMulticast(multicastMsg);
        	break;
        case PRM_NAK_MULTICAST:
        	multicastMsg = check_and_cast<PrmMulticastMessage*>(msg);
        	handlePrmNakMulticast(multicastMsg);
        	break;
        case PRM_RANDOM_WALK:
        	walkMsg = check_and_cast<PrmRandomWalkMessage*>(msg);
        	handlePrmRandomWalk(walkMsg);
			break;
        case PRM_RANDOM_WALK_RESPONSE:
        	walkMsg = check_and_cast<PrmRandomWalkMessage*>(msg);
        	handlePrmRandomWalkResponse(walkMsg);
			break;
        default:
        	Nice::handleUDPMessage(msg);
        }
	}    else {
        delete msg;
    }
} // handleUDPMessage

void Prm::changeState(int toState) {
	Nice::changeState(toState);
//	if (toState == READY) {
//		startPrmRandomWalkDiscover();
//	}
	if ((rand() % (int)(1/0.2)) == 1){
			startPrmRandomWalkDiscover();
		}
}

void Prm::startPrmRandomWalkDiscover() {
	int layer = getHighestLayer();

	// Only populate r list in layers with getHighestLayer>0
//	if(layer <= 0){
//		return;
//	}

	if (getCluster(layer).getSize() == 1 /*&& layer != 0*/) {
		layer--;
	}

	if (layer >= 0) {
		PrmRandomWalkMessage *prmMsg = new PrmRandomWalkMessage("PRM_RANDOM_WALK");
		prmMsg->setCommand(PRM_RANDOM_WALK);
		prmMsg->setLayer(layer);
		prmMsg->setSrcNode(thisNode);
		prmMsg->setBitLength(PRMRANDOMWALK_L(prmMsg));
		prmMsg->setTtl(TTL);
//		prmMsg->setTtl(1);

		for (int i=0; i < NUM_CHILDS; i++) {
			int index = rand() % getCluster(layer).getSize();
			const TransportAddress& random_member = getCluster(layer).get(index);
			if (random_member != thisNode) {
				//std::cout << thisNode.getIp().str() << " envia para " << random_member << endl;
				sendMessageToUDP(random_member, prmMsg->dup());
				randomWalk_initiated_++;
			}
		}

		delete prmMsg;
	}
}

void Prm::sendPrmRandomWalk(PrmRandomWalkMessage* msg) {
	int layer = msg->getLayer();
//	int layer = getHighestLayer();

//	if (getCluster(layer).getSize() == 1 /*&& layer >= 0*/) {
//		layer--;
//	}

	if (layer >= 1) {
		// Stay in the same layer or go down 1 layer with 0.2 probability.
		if ((rand() % 5) == 1) {
			layer--;
		}
	}

	int highestLayer = getHighestLayer();
	if (layer > highestLayer) {
		std::cout << "thisNode:" << thisNode.getIp().str() << " srcNode:" << msg->getSrcNode() << " layer:" << layer << " highestLayer:"<< highestLayer << " ttl:" << (msg->getTtl()+1)<< endl;
		layer = highestLayer;
	}

	if (layer >= 0) {
//		if (thisNode.getIp().str().compare("1.0.0.10") == 0 && msg->getSrcNode().getIp().str().compare("1.0.0.51") == 0) {
//			std::cout << "SIM TTL:"<< msg->getTtl() << " layer:" << layer<< endl;
//		}
		for (int i=0; i < NUM_CHILDS; i++) {
			int index = rand() % getCluster(layer).getSize();
//			if (thisNode.getIp().str().compare("1.0.0.10") == 0 && msg->getSrcNode().getIp().str().compare("1.0.0.51") == 0) {
//				std::cout << "index:"<< index << endl;
//			}
			const TransportAddress& random_member = getCluster(layer).get(index);
//			if (thisNode.getIp().str().compare("1.0.0.10") == 0 && msg->getSrcNode().getIp().str().compare("1.0.0.51") == 0) {
//				std::cout << "random_member:"<< random_member << endl;
//			}
			if (random_member != thisNode && random_member != msg->getSrcNode()) {
//				if (thisNode.getIp().str().compare("1.0.0.51") == 0) {
//				std::cout << thisNode.getIp().str() << " RENCAMINHA para " << random_member << endl;
//				}
				PrmRandomWalkMessage *prmMsg = msg->dup();
				prmMsg->setLayer(layer);
				sendMessageToUDP(random_member, prmMsg);
				randomWalk_forwards_++;
				break;
			}
		}
	}
	delete msg;
}


void Prm::handlePrmRandomWalk(PrmRandomWalkMessage* msg) {
	if (msg->getSrcNode() != thisNode) {
		int ttl = msg->getTtl();

//		if (thisNode.getIp().str().compare("1.0.0.10") == 0 ) {
//			std::cout << thisNode.getIp().str() << " recebe de " << msg->getSrcNode() << " TTL:" << ttl << endl;
//		}

		if (ttl == 0) {
			PrmRandomWalkMessage *prmMsg = new PrmRandomWalkMessage("PRM_RANDOM_WALK_RESPONSE");
			prmMsg->setCommand(PRM_RANDOM_WALK_RESPONSE);
			prmMsg->setLayer(-1);
			prmMsg->setSrcNode(thisNode);
			prmMsg->setBitLength(PRMRANDOMWALK_L(prmMsg));
			prmMsg->setTtl(-1);
			sendMessageToUDP(msg->getSrcNode(), prmMsg);
		} else {
			PrmRandomWalkMessage *prmMsg = msg->dup();
			prmMsg->setTtl(--ttl);
			sendPrmRandomWalk(prmMsg);
		}
	}
	delete msg;
}

void Prm::handlePrmRandomWalkResponse(PrmRandomWalkMessage* msg){
	const TransportAddress& srcNode = msg->getSrcNode();
	if (srcNode != thisNode) {
		bool contains = find(childs_.begin(), childs_.end(), srcNode) != childs_.end();
		if (!contains){
			if (childs_.size() == NUM_CHILDS)  {
				childs_.remove(childs_.front());
			}
			childs_.push_back(srcNode);
//			std::cout << "recebe filho " << childs_.size() <<  endl;
		}
	}
	delete msg;
}

void Prm::finishOverlay(){
	long total_pkts_sent = missing_pkts_.size() + received_pkts_;
	double loss_percentage = 0;

	if (total_pkts_sent > 0){
		loss_percentage = (double)missing_pkts_.size()/total_pkts_sent;
	}

	recordScalar("Packets Received",received_pkts_,NULL);
	recordScalar("Packets Duplicated",duplicated_pkts_,NULL);
	recordScalar("Packets Missing",missing_pkts_.size(),NULL);
	recordScalar("Loss Percentage",loss_percentage,NULL);
	recordScalar("Naks Received",naks_received_,NULL);
	recordScalar("Naks Sent",naks_sent_,NULL);
	recordScalar("randomWalk forward number",randomWalk_forwards_,NULL);
	recordScalar("randomWalk initiated Sent",randomWalk_initiated_,NULL);
	recordScalar("Packets Mesh Link Sent",mesh_sent_pkts_,NULL);

	Nice::finishOverlay();
}

}
