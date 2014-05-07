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

//In dsdv-routing-protocol.cc:110, change m_periodicUpdateInterval to .25
//In dsdv-routing-protocol.h:122, moce m_routingTable to public

NS_LOG_COMPONENT_DEFINE ("AdhocSetup");

double DIST_LIMIT_SQRT = 100;
int totalAttempted, totalSuccess;


using namespace ns3;

static void GenerateTraffic (Ptr<Socket> sockets[], int numAttempts,//DsdvHelper dsdv, 
                             NodeContainer nc );



void ReceivePacket (Ptr<Socket> socket){
  NS_LOG_UNCOND ("Received one packet!");
  totalSuccess++;
}

static void sendPacket (Ptr<Socket> socket){
	int pktSize = 1000;
	socket -> Send(Create<Packet> (pktSize));
}

static void moveBack (std::vector<int> changed, NodeContainer nc, 
	int numAttempts, Ptr<Socket> sources[]){
	
	Ptr<MobilityModel> mobility; 
	Vector pos;

	for (int i = 1; i < nc.GetN(); i++){
		if (changed[i] == 1){
			mobility = nc.Get(i)->GetObject<MobilityModel> ();
			pos = mobility->GetPosition();

			pos.x -= 2000;
			pos.y -= 2000;
			mobility->SetPosition(pos);
		}
	}

	numAttempts--;
	if (numAttempts > 0)
		Simulator::Schedule(Seconds(7.0), &GenerateTraffic,
			sources, numAttempts, nc);
}

static void aGenerateTraffic (Ptr<Socket> sockets[], int numAttempts, 
                             std::vector<int> changed,
                             NodeContainer nc ){

std::cout<<"HERE"<<std::endl;

	/*Ptr<OutputStreamWrapper> routingStream =
		Create<OutputStreamWrapper> ("PostRoutes", std::ios::out);
	dsdv.PrintRoutingTableAllAt (Seconds (0.0), routingStream);*/

	Ptr<MobilityModel> mobility; 
	Vector pos;

	for (int i =1 ; i < nc.GetN(); i++){
		if (changed[i] == 0){
			mobility = nc.Get(i)->GetObject<MobilityModel> ();
			pos = mobility->GetPosition();

			std::cout << "Node " << i << " is at: x: " << pos.x << "; y: " <<pos.y << std::endl;

			//sockets[i-1]->Send (Create<Packet> (pktSize));
			Simulator::Schedule (Seconds(.25*i), &sendPacket, sockets[i-1]);
			totalAttempted++;
		}
	}

	Simulator::Schedule (Seconds(7.0), &moveBack, changed, nc, numAttempts, sockets);

}


static void GenerateTraffic (Ptr<Socket> sockets[], int numAttempts,//DsdvHelper dsdv, 
                             NodeContainer nc )
{

	/*Ptr<OutputStreamWrapper> routingStream =
		Create<OutputStreamWrapper> ("OriginalRoutes", std::ios::out);
	dsdv.PrintRoutingTableAllAt (Seconds (0.0), routingStream);*/


	Ptr<MobilityModel> mobility; 
	Vector pos;

	std::vector<int> changed;
	for (int i = 0; i < nc.GetN(); i++)
		changed.push_back(0);	

	//int changed[25];// = malloc( (int) nc.GetN() * sizeof(int));
	//for (int i = 0; i < nc.GetN(); i++)
	//	changed[i] = 0;	

	for (int i = 1; i < nc.GetN(); i++){
		mobility = nc.Get(i)->GetObject<MobilityModel> ();
		pos = mobility->GetPosition();

		if (pow(pos.x, 2) + pow(pos.y, 2) > pow(DIST_LIMIT_SQRT, 2)) {
			std::cout << "Node # " << i << " is outside of region." << std::endl;

			changed[i] = 1;

			pos.x += 2000;
			pos.y += 2000;
			mobility->SetPosition(pos);
		}
	}

	Simulator::Schedule (Seconds(10.0), &aGenerateTraffic, 
	                   sockets, numAttempts, changed, nc);

}

int main(int argc, char const *argv[]){

std::cout << "May 6, 2014" << std::endl;


	std::string phyMode ("DsssRate1Mbps");
	int packetSize = 1000; // bytes
	int numPackets = 1000;
	Time interPacketInterval = Seconds (1.0);
	int numNodes = 51;
	uint32_t sourceNode = numNodes-1;
	uint32_t sinkNode = 0; //sink node is MBS node 0
	double distance = 40; // m


	NodeContainer nc;
	nc.Create(numNodes);

	WifiHelper wifi;
	//wifi.EnableLogComponents();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
	

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	//wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
	wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
	//wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel");
	//wifiChannel.AddPropagationLoss ("ns3::BuildingsPropagationLossModel");
	//wifiChannel.SetNext("ns3::LogDistancePropagationLossModel");


	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	
	//all parameters below are set to the defaults:
	int pwr = 16.0206;
	wifiPhy.Set ("TxPowerStart", DoubleValue(pwr));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(pwr));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set ("TxGain", DoubleValue(1));
	wifiPhy.Set ("RxGain", DoubleValue(1));
	wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-96));
	wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-99));

	wifiPhy.SetChannel (wifiChannel.Create ());


	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac");


	NetDeviceContainer devices;
	devices = wifi.Install (wifiPhy, wifiMac, nc);


	MobilityHelper mobilityMBS;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
	positionAlloc->Add(Vector(0, 0, 5)); //node 0 should be center and 75m high
	mobilityMBS.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityMBS.Install(nc.Get(0));

	MobilityHelper mobilityUsers;
	/*mobilityUsers.SetPositionAllocator ("ns3::GridPositionAllocator",
	                             "MinX", DoubleValue (-150.0),
	                             "MinY", DoubleValue (-150.0),
	                             "DeltaX", DoubleValue (distance),
	                             "DeltaY", DoubleValue (distance),
	                             "GridWidth", UintegerValue (5),
	                             "LayoutType", StringValue ("RowFirst"));*/
	//mobilityUsers.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityUsers.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
										"X", StringValue("ns3::UniformRandomVariable[Min=-1000|Max=1000]"),
										"Y", StringValue("ns3::UniformRandomVariable[Min=-1000|Max=1000]") );
	mobilityUsers.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
		"Bounds", RectangleValue(Rectangle(-5000, 5000, -5000, 5000)));
	for (int i = 1; i < numNodes; i++)
		mobilityUsers.Install(nc.Get(i));


	DsdvHelper dsdv;
	InternetStackHelper internet;
	internet.SetRoutingHelper (dsdv);
	internet.Install (nc);

	Ipv4AddressHelper ipv4;
	NS_LOG_INFO ("Assign IP Addresses.");
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);


	//
	// Application:
	//

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<Socket> recvSink = Socket::CreateSocket (nc.Get(sinkNode), tid);
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
	recvSink->Bind (local);
	recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

	Ptr<Socket> sources[51];
	InetSocketAddress remote = InetSocketAddress (ifcont.GetAddress (sinkNode, 0), 80);
	for (int i = 1; i < numNodes; i++){
		//Ptr<Socket> source = Socket::CreateSocket (nc.Get (sourceNode), tid);
		sources[i-1] = Socket::CreateSocket (nc.Get (i), tid);
		sources[i-1]->Connect (remote);
	}

	//AsciiTraceHelper ascii;
	//wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
	//wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);


	Simulator::Schedule(Seconds(30.0), &GenerateTraffic,
		sources, 2, nc);

	Simulator::Stop (Seconds (150.0));
	Simulator::Run ();
	Simulator::Destroy ();

std::cout << totalAttempted << "  " << totalSuccess << std::endl;
	return 0;
}