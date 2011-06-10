/*
 * Prm.h
 *
 *  Created on: Jun 5, 2011
 *      Author: tproenca
 */

#ifndef PRM_H_
#define PRM_H_

#include <list>
#include "Nice.h"
#include "PrmMessage_m.h"

namespace oversim {

class MulticastMessageQueue : public std::list<PrmMulticastMessage*> {
	/* Size of the packet buffer.*/
	const static int BUFFER_SIZE = 20;
public:
	MulticastMessageQueue();
	void push(PrmMulticastMessage* x);
	inline void pop() { return std::list<PrmMulticastMessage*>::pop_front(); }
};

class Prm: public Nice {
//	const static string SIM_RESULTS_FILENAME = "prm-results";
	const static int NUM_CHILDS = 3; // Size of the list of Mesh links (a.k.a  "r")
	const static int TTL = 2; // e.g. TTL =3, RANDOMWALK message will we forwarded 3 times before a node replies
	const static int DISCOVERY_INTERVAL = 80;
	const static double BETA = 0.01;
private:
	/* Buffer to keep the last BUFFER_SIZE packets received. */
	MulticastMessageQueue pkt_buffer_;
	/* Store the sequence number of the missing packets. */
	std::list<int> missing_pkts_;
	/* The sequence number of the last packet received. */
	long last_pkt_id_;
	/* Number of duplicated packets received. */
	long duplicated_pkts_;
	/* Number of packets received successfully. */
	long received_pkts_;
	/* Responsible for recording the simulation results. */
	//cOutVector sim_results_;
	/* Sequence number counter. */
	long seqNo_;
	/* Number of naks received. */
	long naks_received_;
	/* Number of naks sent. */
	long naks_sent_;
	/* Number of random walks forwarded sent. */
	long randomWalk_forwards_;
	/* Number of random walks initiated. */
	long randomWalk_initiated_;
	/* Number of Multicast packets sent over mesh links. */
	long mesh_sent_pkts_;

	/* List of childs. Used for in the PRM random walk scheme. */
	std::list<TransportAddress> childs_;

protected:
	void printNodeStatus(PrmMulticastMessage* msg);
	void updatePrmMulticast(PrmMulticastMessage* msg);
	void handlePrmMulticast(PrmMulticastMessage* multicastMsg);
	void handlePrmNakMulticast(PrmMulticastMessage* msg);
	void handlePrmRandomWalk(PrmRandomWalkMessage* msg);
	void handlePrmRandomWalkResponse(PrmRandomWalkMessage* msg);
	void sendPrmNakMulticast(PrmMulticastMessage* msg);
	void startPrmRandomWalkDiscover();
	void sendPrmRandomWalk(PrmRandomWalkMessage* msg);
	void sendDataToChilds(PrmMulticastMessage* msg);
	virtual void changeState(int toState);
public:
	Prm();
	virtual ~Prm();
	virtual void handleAppMessage(cMessage* msg);
	virtual void handleUDPMessage(BaseOverlayMessage* msg);
	virtual void finishOverlay();
};

}

#endif /* PRM_H_ */
