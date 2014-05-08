#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/dsdv-helper.h"
#include "ns3/dsdv-rtable.h"
#include "ns3/dsdv-routing-protocol.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-interface-address.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>

//In dsdv-routing-protocol.cc:110, change m_periodicUpdateInterval
//In dsdv-routing-protocol.h:122, moce m_routingTable to public

NS_LOG_COMPONENT_DEFINE ("AdhocSetup");

double DIST_LIMIT_SQRT = 400;
uint32_t bytesTotal = 0;
uint32_t packetsReceived = 0;
int numNodes;
using namespace ns3;

void ReceivePacket (Ptr<Socket> socket){
	//NS_LOG_UNCOND ( Simulator::Now().GetSeconds() << " Received one packet!");
	Ptr <Packet> packet;
	while ((packet = socket->Recv ())){
		bytesTotal += packet->GetSize ();
		packetsReceived += 1;
	}
}

static void sendPacket (Ptr<Socket> socket){
	int pktSize = 1000;
	socket -> Send(Create<Packet> (pktSize));
}

static void GenerateTraffic (Ptr<Socket> socket[],Time pktInterval, int pktCount)
{
	if(pktCount>0){
		for(int i=0;i<numNodes;i++){
			Simulator::Schedule (Seconds(.2*i), &sendPacket, socket[i]);
			//NS_LOG_UNCOND(Simulator::Now().GetSeconds()<<": "<<i<<" sent a packet");
		}
		Simulator::Schedule(pktInterval, &GenerateTraffic,
			socket, pktInterval, pktCount-1);	
	}
	else {
		for(int i=0;i<numNodes;i++){
			socket[i]->Close();
		}
		NS_LOG_UNCOND("Done!");
	}

	
}

int main(int argc, char *argv[]){
	double pwr = 1;
	int pktCount = 1;
	numNodes = 50;

	CommandLine cmd;
	cmd.AddValue ("power", "Power", pwr);
	cmd.Parse (argc, argv);

	std::string phyMode ("DsssRate1Mbps");
	//int packetSize = 1000; // bytes
	double interval = .2*numNodes+1;
	Time interPacketInterval = Seconds (interval);
	int outOfBounds = 200;
	double totalTime = interval*pktCount+10;
	NS_LOG_UNCOND("Time="<<totalTime);

	NodeContainer sources;
	NodeContainer MBS;
	
	MBS.Create(1);
	sources.Create(numNodes);
	NodeContainer nc(MBS,sources);

	WifiHelper wifi;
	//wifi.EnableLogComponents();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
	

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");

	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	
	//all parameters below are set to the defaults:
	wifiPhy.Set ("TxPowerStart", DoubleValue(pwr));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(pwr));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set ("TxGain", DoubleValue(0));
	wifiPhy.Set ("RxGain", DoubleValue(0));
	wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-80));
	wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-80));

	wifiPhy.SetChannel (wifiChannel.Create ());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac");

	NetDeviceContainer devices = wifi.Install(wifiPhy,wifiMac,nc);

	MobilityHelper mobilityMBS;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
	positionAlloc->Add(Vector(0, 0, 5)); //node 0 should be center and 75m high
	mobilityMBS.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityMBS.Install(MBS);

	MobilityHelper mobilityUsers;
	  ObjectFactory pos;
	  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
	  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=-20.0|Max=20.0]"));
	  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=-20.0|Max=20.0]"));

	  Ptr <PositionAllocator> taPositionAlloc = pos.Create ()->GetObject <PositionAllocator> ();

	//mobilityUsers.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityUsers.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
		"Bounds", RectangleValue(Rectangle(-200, 200, -200, 200)));
	mobilityUsers.SetPositionAllocator(taPositionAlloc);
	mobilityUsers.Install(sources);
	
	InternetStackHelper internet;
	internet.Install (nc);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);

	//
	// Application:
	//

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<Socket> recvSink = Socket::CreateSocket (nc.Get(0), tid);
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
	recvSink->Bind (local);
	recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));


//	NS_LOG_INFO("Sink: "<<nc.Get(0)->GetId()<<": ");

//	ifcont.GetAddress(0).Print(std::cout);


	Ptr<Socket> sockets[numNodes];
	InetSocketAddress remote = InetSocketAddress (ifcont.GetAddress (0), 80);
	for (int i = 0; i < numNodes; i++){
		sockets[i] = Socket::CreateSocket (sources.Get (i), tid);
		sockets[i]->Connect (remote);

		/*std::cout<<std::endl<<sockets[i]->GetNode()->GetId()<<": ";
		ifcont.GetAddress(i+1).Print(std::cout);
		*/
	}

	/*AsciiTraceHelper ascii;
	wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
	wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
	*/

	Simulator::Schedule(Seconds(3.0), &GenerateTraffic,
		sockets, interPacketInterval,pktCount);

	Simulator::Stop (Seconds (totalTime));
	Simulator::Run ();
	Simulator::Destroy ();

	//std::cout<<"Bytes Recvd = "<<bytesTotal<<std::endl;
	double totalPackets = numNodes*pktCount;
	std::cout<<"Power = "<<pwr<<std::endl;
	std::cout<<"Packets Recvd = "<<packetsReceived<<std::endl;
	std::cout<<"Total Packets = "<<totalPackets<<"="<<numNodes<<"*"<<pktCount<<std::endl;
	std::cout<<"Percent = "<<packetsReceived*100/totalPackets<<std::endl;
	return 0;
}