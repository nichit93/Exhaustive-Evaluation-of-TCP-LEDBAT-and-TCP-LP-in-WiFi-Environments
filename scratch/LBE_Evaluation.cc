#include <cmath>
#include <iostream>
#include <sstream>
// ns3 include
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/gnuplot.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("Exhaustive Evaluation of TCP-LEDBAT and TCP-LP");


uint32_t maxBytes = 102400000;
uint64_t lastTotalRx0 = 0,lastTotalRx1 = 0,lastTotalRx2 = 0;
double aggregate=0,f0=0,f1=0,f2=0;
Ptr<PacketSink> sink0;
Ptr<PacketSink> sink1;
Ptr<PacketSink> sink2;
std::ofstream outfile0,outfile1,outfile2;



void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur0 = (sink0->GetTotalRx () - lastTotalRx0) * (double) 8 /1e5;
  double cur1 = (sink1->GetTotalRx () - lastTotalRx1) * (double) 8/1e5;
  double cur2 = (sink2->GetTotalRx () - lastTotalRx2) * (double) 8/1e5;
  outfile0 << now.GetSeconds () << " " << cur0 << std::endl;
  outfile1 << now.GetSeconds () << " " << cur1 << std::endl;
  outfile2 << now.GetSeconds () << " " << cur2 << std::endl; 
  aggregate+=cur0+cur1+cur2; 
  f0+=cur0; f1+=cur1; f2+=cur2;   
 // std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" <<std::endl;
  lastTotalRx0 = sink0->GetTotalRx ();
  lastTotalRx1 = sink1->GetTotalRx ();  
  lastTotalRx2 = sink2->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int 
main (int argc, char *argv[])
{ 
  double simulationTime = 20;
  uint32_t nleft=3;
  uint32_t nright=3;
  std::string tcp = "lp";
  NodeContainer          m_leftLeaf;            //!< Left Leaf nodes
  NetDeviceContainer     m_leftLeafDevices;     //!< Left Leaf NetDevices
  NodeContainer          m_rightLeaf;           //!< Right Leaf nodes
  NetDeviceContainer     m_rightLeafDevices;    //!< Right Leaf NetDevices
  NodeContainer          centrallink;             //!< Router and AP
  NetDeviceContainer     m_centralDevices;       //!< Routers NetDevices
  NetDeviceContainer     m_rightRouterDevices;
  Ipv4InterfaceContainer m_leftLeafInterfaces;    //!< Left Leaf interfaces (IPv4)
  Ipv4InterfaceContainer m_leftRouterInterfaces;  //!< Left router interfaces (IPv4)
  Ipv4InterfaceContainer m_rightLeafInterfaces;   //!< Right Leaf interfaces (IPv4)
  Ipv4InterfaceContainer m_rightRouterInterfaces; //!< Right router interfaces (IPv4)
  Ipv4InterfaceContainer m_routerInterfaces;      //!< Router interfaces (IPv4)
  WifiHelper wifi;
  MobilityHelper mobility;
  NodeContainer ap;
  WifiMacHelper wifiMac;
  NodeContainer wifiApNode;
  NetDeviceContainer apDevice;
  
  CommandLine cmd;
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);
    
  // Create the BottleNeck Routers
  centrallink.Create(2);

  // Create the leaf nodes
  m_leftLeaf.Create (nleft);
  m_rightLeaf.Create (nright);

  PointToPointHelper rightHelper,centrallinkHelper;

  centrallinkHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  centrallinkHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // Add the link connecting routers
  m_centralDevices = centrallinkHelper.Install (centrallink);
  wifiApNode = centrallink.Get (0);
 
  // Add the left side links
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  //wifiPhy.SetErrorRateModel ("ns3::DsssErrorRateModel");
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  Ssid ssid = Ssid ("wifi-default");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  
  // setup stas.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));

  m_leftLeafDevices = wifi.Install (wifiPhy, wifiMac, m_leftLeaf);
  
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  apDevice = wifi.Install (wifiPhy, wifiMac, wifiApNode);
  
  // mobility.
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  
  //Left leaf nodes Mobility
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel");
  mobility.Install (m_leftLeaf);
  
  //AP Mobility
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  
  // Add the right side links
  rightHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  rightHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));
  for (uint32_t i = 0; i < nright; ++i)
    {
      NetDeviceContainer c = rightHelper.Install (centrallink.Get (1),
                                                  m_rightLeaf.Get (i));
      m_rightRouterDevices.Add (c.Get (0));
      m_rightLeafDevices.Add (c.Get (1));
    }  
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));


  InternetStackHelper stack;
  stack.Install (centrallink);
  stack.Install (m_leftLeaf);
  stack.Install (m_rightLeaf);
  //std::stringstream s;
  //s<< m_leftLeaf.Get(2)->GetId();
  //Config::Set ( "$ns3::NodeListPriv/NodeList/" + s.str() + "/$ns3::TcpL4Protocol/SocketType",TypeIdValue (TcpLedbat::GetTypeId ()));
  Config::Set ("$ns3::NodeListPriv/NodeList/2/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
  //Config::Set ("$ns3::NodeListPriv/NodeList/3/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
  //Config::Set ("$ns3::NodeListPriv/NodeList/4/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TcpVegas::GetTypeId ()));

 
  //Assigns IPAddress
  Ipv4AddressHelper routerIp;
  Ipv4AddressHelper rightIp;
  Ipv4AddressHelper leftIp;
  routerIp.SetBase ("10.1.1.0", "255.255.255.0");
  m_routerInterfaces = routerIp.Assign (m_centralDevices);
 
  // Assign to left side 
  leftIp.SetBase("10.1.2.0","255.255.255.0");
  NetDeviceContainer ndc;
  ndc.Add (m_leftLeafDevices);
  ndc.Add(apDevice);
  Ipv4InterfaceContainer ifc = leftIp.Assign (ndc);
   for (uint32_t i = 0; i < m_leftLeaf.GetN (); ++i)
   {
      m_leftLeafInterfaces.Add (ifc.Get (i));
    }
  m_leftRouterInterfaces.Add (ifc.Get (m_leftLeaf.GetN ()));
  leftIp.NewNetwork ();

  // Assign to right side 
  rightIp.SetBase("10.1.3.0","255.255.255.0");
  for (uint32_t i = 0; i < m_rightLeaf.GetN (); ++i)
    { 
      NetDeviceContainer ndc;
      ndc.Add (m_rightLeafDevices.Get (i));
      ndc.Add (m_rightRouterDevices.Get (i));
      Ipv4InterfaceContainer ifc = rightIp.Assign (ndc);
      m_rightLeafInterfaces.Add (ifc.Get (0));
      m_rightRouterInterfaces.Add (ifc.Get (1));
      rightIp.NewNetwork ();
    }
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //BulksendApplication on Left nodes
  BulkSendHelper source ("ns3::TcpSocketFactory", Address ());
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

  ApplicationContainer sourceApps;

  for (uint32_t i = 0; i < nleft; ++i)
    {
      AddressValue sourceaddress (InetSocketAddress (m_rightLeafInterfaces.GetAddress (i), 8080));
      source.SetAttribute ("Remote", sourceaddress);
      sourceApps.Add (source.Install (m_leftLeaf.Get (i)));
    }
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (20.0));
  //PacketsinkHelper on right nodes
  PacketSinkHelper sinkhelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8080));
  ApplicationContainer sinkApps;
  
  
  for (uint32_t i = 0; i < nright; ++i)
    {
      sinkApps.Add (sinkhelper.Install (m_rightLeaf.Get (i)));
    }
  
  sink0 = StaticCast<PacketSink> (sinkApps.Get (0));
  sink1 = StaticCast<PacketSink> (sinkApps.Get (1));
  sink2 = StaticCast<PacketSink> (sinkApps.Get (2));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (20.0));

  
  std::ostringstream tss;
  tss << "Flow0.plt";

 
  outfile0.open (tss.str().c_str(), std::ofstream::out);

  outfile0<< "set terminal png" <<"\n";
  outfile0<< "set output \"" << "Flow0.png" <<"\"\n"; 
  outfile0<< "set title \"" << "Flow0" << "\"\n";
  outfile0<< "set xlabel \"X Values\"\n";
  outfile0<< "set ylabel \"Y Values\"\n\n";
  outfile0<< "set xrange [0:20]\n";
  outfile0<< "set yrange [0:10]\n";
  outfile0<<"plot \"-\"  title \"Throughput\" with linespoints\n";
  std::ostringstream tss1;
  tss1 << "Flow1.plt";

 
  outfile1.open (tss1.str().c_str(), std::ofstream::out);

  outfile1<< "set terminal png" <<"\n";
  outfile1<< "set output \"" << "Flow1.png" <<"\"\n"; 
  outfile1<< "set title \"" << "Flow1" << "\"\n";
  outfile1<< "set xlabel \"X Values\"\n";
  outfile1<< "set ylabel \"Y Values\"\n\n";
  outfile1<< "set xrange [0:20]\n";
  outfile1<< "set yrange [0:10]\n";
  outfile1<<"plot \"-\"  title \"Throughput\" with linespoints\n";

  std::ostringstream tss2;
  tss2 << "Flow2.plt";

 
  outfile2.open (tss2.str().c_str(), std::ofstream::out);

  outfile2<< "set terminal png" <<"\n";
  outfile2<< "set output \"" << "Flow2.png" <<"\"\n"; 
  outfile2<< "set title \"" << "Flow2" << "\"\n";
  outfile2<< "set xlabel \"X Values\"\n";
  outfile2<< "set ylabel \"Y Values\"\n\n";
  outfile2<< "set xrange [0:20]\n";
  outfile2<< "set yrange [0:10]\n";
  outfile2<<"plot \"-\"  title \"Throughput\" with linespoints\n";



  
  Simulator::Schedule (Seconds (0), &CalculateThroughput);
  NS_LOG_INFO ("Run Simulation.");
  
  Simulator::Stop(Seconds(simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();
  outfile0 <<"e\n";
  outfile0.close ();
  outfile1 <<"e\n";
  outfile1.close ();
  outfile2 <<"e\n";
  outfile2.close ();
  system("gnuplot Flow0.plt");
  system("gnuplot Flow1.plt");
  system("gnuplot Flow2.plt");
  aggregate=aggregate/(double)200 ;
  f0=f0/(double)200;
  f1=f1/(double)200;
  f2=f2/(double)200;
  std::cout<<"aggregate throughput:"<<aggregate<<"\nFlow0:"<<f0<<"\nFlow1:"<<f1<<"\nFlow2:"<<f2;
  return 0;
}
 
