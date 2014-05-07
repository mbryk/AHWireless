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

using namespace ns3;

void ReceivePacket (Ptr<Socket> socket){
  NS_LOG_UNCOND ("Received one packet!");
}

static void GenerateTraffic (Ptr<Socket> socket[20],Time pktInterval)
{
	int pktSize = 1000;

	std::cout<<"HERE"<<std::endl;

	for(int i=0;i<9;i++){
		socket[i]->Send (Create<Packet> (pktSize));
		std::cout<<"HERE"<<i<<std::endl;
	}
	Simulator::Schedule(Seconds(pktInterval), &GenerateTraffic,
		socket, pktInterval);
}

int main(int argc, char const *argv[]){

std::cout << "May 6, 2014" << std::endl;


	std::string phyMode ("DsssRate1Mbps");
	//int packetSize = 1000; // bytes
	Time interPacketInterval = Seconds (1.0);
	int numNodes = 10;
	//uint32_t sourceNode = numNodes-1;
	uint32_t sinkNode = 0; //sink node is MBS node 0
	double distance = 30; // m
	int outOfBounds = 1000;

	NodeContainer sources;
	NodeContainer MBS;
	
	sources.Create(numNodes-1);
	MBS.Create(1);
	NodeContainer nc(MBS,sources);

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
	double pwr = 25.0;
	wifiPhy.Set ("TxPowerStart", DoubleValue(pwr));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(pwr));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set ("TxGain", DoubleValue(1));
	wifiPhy.Set ("RxGain", DoubleValue(1));
	wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-96));
	wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-99));

	wifiPhy.SetChannel (wifiChannel.Create ());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	Ssid ssid = Ssid ("wifi-default");

	wifiMac.SetType ("ns3::StaWifiMac",
					"Ssid", SsidValue(ssid),
					"ActiveProbing", BooleanValue(false));

	NetDeviceContainer MBSdevice = wifi.Install(wifiPhy,wifiMac,MBS);
	NetDeviceContainer devices = MBSdevice;

	wifiMac.SetType ("ns3::ApWifiMac",
					"Ssid", SsidValue(ssid));
	NetDeviceContainer sourceDevices = wifi.Install(wifiPhy,wifiMac,sources);
	devices.Add(sourceDevices);

	MobilityHelper mobilityMBS;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
	positionAlloc->Add(Vector(outOfBounds, outOfBounds, 5)); //node 0 should be center and 75m high
	mobilityMBS.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityMBS.Install(MBS);

	MobilityHelper mobilityUsers;
	mobilityUsers.SetPositionAllocator ("ns3::GridPositionAllocator",
	                             "MinX", DoubleValue (-10.0),
	                             "MinY", DoubleValue (0.0),
	                             "DeltaX", DoubleValue (distance),
	                             "DeltaY", DoubleValue (distance),
	                             "GridWidth", UintegerValue (20),
	                             "LayoutType", StringValue ("RowFirst"));
	mobilityUsers.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	//mobilityUsers.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
	//	"Bounds", RectangleValue(Rectangle(-1000, 1000, -1000, 1000)));
	mobilityUsers.Install(sources);
	
	InternetStackHelper internet;
	internet.Install (nc);

	Ipv4AddressHelper ipv4;
	NS_LOG_INFO ("Assign IP Addresses.");
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

	Ptr<Socket> sockets[9];
	InetSocketAddress remote = InetSocketAddress (ifcont.GetAddress (sinkNode, 0), 80);
	for (int i = 1; i < numNodes; i++){
		sockets[(i-1)] = Socket::CreateSocket (nc.Get (i), tid);
		sockets[(i-1)]->Connect (remote);
	}

	AsciiTraceHelper ascii;
	wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
	wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);

	Simulator::Schedule(Seconds(3.0), &GenerateTraffic,
		sockets, interPacketInterval);

	Simulator::Stop (Seconds (15.0));
	Simulator::Run ();
	Simulator::Destroy ();


	return 0;
}