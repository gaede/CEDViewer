/** DSTViewer - event display for DST files
 *  @author: S.Daraszewicz
 *  @date: 15.08.08
 **/
#ifdef USE_CLHEP 

#include "DSTViewer.h"
#include <EVENT/ReconstructedParticle.h>
#include <iostream>
#include "ClusterShapes.h"

#include "HelixClass.h"
#include <math.h>
#include "MarlinCED.h"

#include <gear/GEAR.h>
#include <gear/BField.h>
#include <gear/TPCParameters.h>
#include <gear/PadRowLayout2D.h>
#include <gearimpl/Vector3D.h>
#include "ColorMap.h"

using namespace lcio ;
using namespace marlin ;

/** Define the layers in the simulation */
#define PION_LAYER		(1<<CED_LAYER_SHIFT)
#define PHOTON_LAYER	(2<<CED_LAYER_SHIFT)
#define NEUTRON_LAYER	(3<<CED_LAYER_SHIFT)		
#define NHADR_LAYER  	(4<<CED_LAYER_SHIFT)
#define CHADR_LAYER  	(5<<CED_LAYER_SHIFT)
#define TPC_LAYER    	(6<<CED_LAYER_SHIFT)
#define ECAL_LAYER   	(7<<CED_LAYER_SHIFT)
#define HCAL_LAYER   	(8<<CED_LAYER_SHIFT)
#define CLUSTER_LAYER	(9<<CED_LAYER_SHIFT)

/** Jet layers... */
#define JET2_LAYER		(12<<CED_LAYER_SHIFT)
#define JET3_LAYER		(13<<CED_LAYER_SHIFT)
#define JET4_LAYER		(14<<CED_LAYER_SHIFT)
#define JET5_LAYER		(15<<CED_LAYER_SHIFT)
#define JET6_LAYER		(16<<CED_LAYER_SHIFT)
#define MOM_LAYER		(17<<CED_LAYER_SHIFT)
#define BACKUP_LAYER	(18<<CED_LAYER_SHIFT)


DSTViewer aDSTViewer ;

/**
 * DSTViewer description
 **/	
DSTViewer::DSTViewer() : Processor("DSTViewer") {

	_description = "CED based event display for DST files";  

	registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			"ParticleCollection" , 
			"Particle Collection Name" , 
			_particleCollection , 
			std::string("RecoParticles"));

	StringVec  jetColNames ;// typedef std::vector< std::string > StringVec ;  
	jetColNames.push_back( "Durham_2Jets" ) ;
	jetColNames.push_back( "Durham_3Jets" ) ;
	jetColNames.push_back( "Durham_4Jets" ) ;
	jetColNames.push_back( "Durham_5Jets" ) ;
	jetColNames.push_back( "Durham_6Jets" ) ;

	registerInputCollections( LCIO::RECONSTRUCTEDPARTICLE,
			"JetCollections" , 
			"Jet Collections Names'" , 
			_jetCollections , 
			jetColNames );

	registerProcessorParameter("MagneticField",
			"Magnetic Field",
			_bField,
			(float)3.5);

	registerProcessorParameter("WaitForKeyboard",
			"Wait for Keyboard before proceed",
			_waitForKeyboard,
			(int)1);

	registerProcessorParameter("LayerReco",
			"Layer for Reco Particles",
			_layerReco,
			(int)9);
}

/**
 * DSTViewer initialise */
void DSTViewer::init() {
	_nRun = -1;
	_nEvt = 0;
	MarlinCED::init(this);
	//ced_client_init("localhost",7286);
	//ced_register_elements();
}


/**
 * Process run header (?) */
void DSTViewer::processRunHeader( LCRunHeader* run) { 
	_nRun++ ;
	_nEvt = 0;
} 

/**
 * Main function processEvent */
void DSTViewer::processEvent( LCEvent * evt ) { 

	// Gets the GEAR global geometry params
	const gear::TPCParameters& gearTPC = Global::GEAR->getTPCParameters() ;
	const gear::PadRowLayout2D& padLayout = gearTPC.getPadLayout() ;

	// this defines the number of layers (?)
	int marker = 9;

	streamlog_out(MESSAGE)  << std::endl;
	std::cout << "(+++ This is the DSTViewer +++)" << std::endl;
	std::cout << "  DSTViewer  : event number " << _nEvt << std::endl;
	std::cout << std::endl;

	_mcpList.clear();

	std::cout << std::endl;
	std::cout << std::endl;

	/**
	 * Drawing Geometry
	 * Reset drawing buffer and START drawing collection
	 */ 
	MarlinCED::newEvent( this, _detModel ) ;

	/** Drawing Reconstructed Particles */
		try {

			LCCollection * col = evt->getCollection(_particleCollection.c_str());
			int nelem = col->getNumberOfElements();

			/** Total E, p, q values for the event */
			float TotEn = 0.0;
			float TotPX = 0.0;
			float TotPY = 0.0;
			float TotPZ = 0.0;
			float TotCharge = 0.0;
			float bField = Global::GEAR->getBField().at(  gear::Vector3D(0,0,0)  ).z() ;

			float ene_min = 0.0;
			float ene_max = 100.0;

			for (int ip(0); ip < nelem; ++ip) {
				ReconstructedParticle * part = dynamic_cast<ReconstructedParticle*>(col->getElementAt(ip));

				//TrackVec trackVec = part->getTracks();
				//int nTracks =  (int)trackVec.size();
				// this is needed for Calorimeter Hits display
				ClusterVec clusterVec = part->getClusters();
				int nClusters = (int)clusterVec.size();

				/** get particle properties */
				float ene = (float) part->getEnergy();
				float px  = (float)(part->getMomentum()[0]);
				float py  = (float)(part->getMomentum()[1]);
				float pz  = (float)(part->getMomentum()[2]);
				const double * pm = part->getMomentum();
				gear::Vector3D pp( pm[0], pm[1] , pm[2] ) ;
				float ptot = pp.r();
				float charge = (float)part->getCharge();
				int type = (int)part->getType();

				//Vertex *startVert = part->getStartVertex();
				//float ver_x = (float) startVert->getPosition()[0];
				//std::cout << "start vertex x = " << startVertx << std::endl;

				// start point
				float refx = 0.0;
				float refy = 0.0;
				float refz = 0.0;

				TotEn += ene;
				TotPX += px;
				TotPY += py;
				TotPZ += pz;
				TotCharge += charge;

				std::cout 	<< "Particle : " << ip
					<< " Type : " << type 
					<< " PX = " << px
					<< " PY = " << py
					<< " PZ = " << pz
					<< " P_tot = " << ptot
					<< " Charge = " << charge
					<< " E  = " << ene << std::endl;




				int ml = marker | TPC_LAYER; // this defines the default layer 1
				if (type == 22){
					ml = marker | ECAL_LAYER; // photons on layer 2
				}
				if (type ==2112){
					ml = marker | NEUTRON_LAYER; //neutrons on layer 3
				}

				int color = returnTrackColor(type);
				int size = returnTrackSize(type);

				/** Draw the helix and straight lines */
				MarlinCED::drawHelix(bField, charge, refx, refy, refz, px, py, pz, ml, size, color, 0.0, padLayout.getPlaneExtent()[1], 
							gearTPC.getMaxDriftLength());
							
				/** Draw momentum lines from the ip */
				char Mscale = 'b'; // 'b': linear, 'a': log
				int McolorMap = 2; //hot: 3
				int McolorSteps = 256;
				// float p = sqrt(px*px+py*py+pz*pz);
				float ptot_min = 0.0;
				float ptot_max = 25.0;
				int Mcolor = returnRGBClusterColor(ptot, ptot_min, ptot_max, McolorSteps, Mscale, McolorMap);
				int LineSize = 1;
				float momScale = 100;
				ced_line(refx, refy, refz, momScale*px, momScale*py, momScale*pz, MOM_LAYER, LineSize, Mcolor);

				if (nClusters > 0 ) {
					// std::cout 	<< "nCluster > 0" << std::endl;
					// std::cout 	<<  clusterVec.size() <<std::endl;

					Cluster * cluster = clusterVec[0];

					double * center  = new double[3];
					float * center_r  = new float[3];
					center[0] = cluster->getPosition()[0];
					center[1] = cluster->getPosition()[1];
					center[2] = cluster->getPosition()[2];

					center_r[0] = (float)cluster->getPosition()[0];
					center_r[1] = (float)cluster->getPosition()[1];
					center_r[2] = (float)cluster->getPosition()[2];

					float phi = cluster->getIPhi();
					float theta = cluster->getITheta();
					float eneCluster = cluster->getEnergy();

					double rotate[] = {0.0, 0.0, 0.0};
					// for particles centered at the origin
					if (refx==0 && refy==0 && refz==0){
						rotate[0] = 0.0;
						rotate[1] = theta*180/M_PI;
						rotate[2] = phi*180/M_PI;
					}
					else {
						std::cout 	<< "This is not yet implemented" << std::endl;
						return;
					} 


					double sizes[] = {100.0, 100.0, 100.0};
					int scale_z = 4;
					sizes[0] = returnClusterSize(eneCluster, ene_min, ene_max);
					sizes[1] = sizes[0];
					sizes[2] = scale_z*returnClusterSize(eneCluster, ene_min, ene_max);

					char scale = 'a'; // 'b': linear, 'a': log
					int colorMap = 3; //jet: 3
					int colorSteps = 256;
					int color = returnRGBClusterColor(eneCluster, ene_min, ene_max, colorSteps, scale, colorMap);


					int cylinder_sides = 30;
					ced_geocylinder_r(sizes[0]/2, sizes[2], center, rotate, cylinder_sides, color, CLUSTER_LAYER);
					ced_hit(center[0],center[1],center[2], 1, (int)(sqrt(2)*sizes[0]/4), color);

					/*
					 * End points for the arrow
					 */

					int sizeLine = 2;
					//int ml_line = 9;
					float radius = (float)0.5*sizes[2];
					float arrow = 0.2*radius;
					int color_arrow = color + 0x009900; //colour of the needle

					float xEnd = center[0] + (radius - arrow) * std::sin(theta)*std::cos(phi);
					float yEnd = center[1] + (radius - arrow) * std::sin(theta)*std::sin(phi);
					float zEnd = center[2] + (radius - arrow) * std::cos(theta);

					float xArrowEnd = center[0] + (radius) * std::sin(theta)*std::cos(phi);
					float yArrowEnd = center[1] + (radius) * std::sin(theta)*std::sin(phi);
					float zArrowEnd = center[2] + (radius) * std::cos(theta);

					// this is the direction arrow
					ced_line(center[0],center[1],center[2], xEnd, yEnd, zEnd, CLUSTER_LAYER, sizeLine, color);					
					ced_line(xEnd, yEnd, zEnd, xArrowEnd, yArrowEnd, zArrowEnd, CLUSTER_LAYER, sizeLine, color_arrow);

					}
				}

				int color = 0x00000000;
				// ---- now we draw the jets (if any ) --------------------
				for(unsigned int i=0;i<_jetCollections.size();++i){ //number of jets

					streamlog_out( DEBUG )  << " drawing jets from collection " << _jetCollections[i] << std::endl ;

					LCCollection * col = evt->getCollection( _jetCollections[i] );
						
						int nelem = col->getNumberOfElements();
						
						//int color = 0x000000;
						
						
						for (int j=0; j < nelem; ++j) { //number of elements in a jet
							ReconstructedParticle * jet = dynamic_cast<ReconstructedParticle*>( col->getElementAt(j) );
							streamlog_out( DEBUG )  <<   "     - jet energy " << jet->getEnergy() << std::endl ;
						
							const double * mom = jet->getMomentum() ;
							//const double jet_ene = jet->getEnergy();
							gear::Vector3D v( mom[0], mom[1] , mom[2] ) ;
							
							int layer = 0;
							const ReconstructedParticleVec & pv = jet->getParticles();
							float pt_norm = 0.0;
							for (unsigned int k = 0; k<pv.size(); ++k){
								const double * pm = pv[k]->getMomentum();
								gear::Vector3D pp( pm[0], pm[1] , pm[2] ) ;
								gear::Vector3D ju = v.unit();
								gear::Vector3D pt = pp - (ju.dot(pp))*ju;
								pt_norm += pt.r();
								
								layer = returnJetLayer(_jetCollections[i]);
								color = returnJetColor(_jetCollections[i], j);
								
								MarlinCED::drawHelix(bField, pv[k]->getCharge(), 0.0, 0.0, 0.0, pm[0], pm[1], pm[2], layer, 1, color, 0.0, padLayout.getPlaneExtent()[1], 
								gearTPC.getMaxDriftLength());
							}
						   					   
	    	 				double center_c[3] = {0., 0., 0. };
							double rotation_c[3] = { 0.,  v.theta()*180./M_PI , v.phi()*180./M_PI };
        					float RGBAcolor[4] = {0.2+0.2*i, 0.2+0.2*i, 1.0-0.25*i, 0.3};
        					if (i==0){
        						RGBAcolor[0] = 1.0;
        						RGBAcolor[1] = 0.3;
        						RGBAcolor[2] = 0.1;
        						RGBAcolor[3] = 0.3;
        					}
        					double scale_pt = 20;
        					double scale_mom = 25;
        					double min_pt = 50;
    						ced_cone_r( min_pt + scale_pt*pt_norm , scale_mom*v.r() , center_c, rotation_c, layer, RGBAcolor);

						}
				}



				char scale = 'a'; // 'b': linear, 'a': log
				int colorMap = 3; //jet: 3
				unsigned int colorSteps = 256;
				unsigned int ticks = 6; //middle 
				DSTViewer::showLegendSpectrum(colorSteps, scale, colorMap, ene_min, ene_max, ticks);

				std::cout 	<< std::endl;
				std::cout 	<< "Total Energy and Momentum Balance of Event" << std::endl;
				std::cout 	<< "Energy = " << TotEn
							<< " PX = " << TotPX
							<< " PY = " << TotPY
							<< " PZ = " << TotPZ
							<< " Charge = " << TotCharge << std::endl;
				std::cout 	<< std::endl;

				std::cout << "Setup properties" << std::endl;
				std::cout << "B_Field = " << bField << " T" << std::endl;

			}
			catch( DataNotAvailableException &e){}
		//}

		/*
		 * This refreshes the view? ...
		 */
		MarlinCED::draw(this, _waitForKeyboard );
		_nEvt++;
	}

	void DSTViewer::check( LCEvent * evt ) { }

	void DSTViewer::end(){ } 

	/**
	 * PRELIM */
	int DSTViewer::returnTrackColor(int type) {

		int kcol = 0x999999; //default black - unknown

		if (type==211) 			kcol = 0x990000; //red - pi^+
		else if (type==-211) 	kcol = 0x996600; //orange - pi^-
		else if (type==22) 		kcol = 0x00ff00; //yellow - photon
		else if (type==11) 		kcol = 0x660066; //dark blue - e^-
		else if (type==-11) 	kcol = 0x660099; //violet - e^+
		else if (type==2112)	kcol = 0x99FFFF; //white - n0
		else if (type==-2112)	kcol = 0x99FFFF; //white n bar
		else {
			std::cout 	<< "Unassigned type of colour: default" << std::endl;
		}
		return kcol;
	}


	int DSTViewer::returnClusterColor(float eneCluster, float cutoff_min, float cutoff_max){

		/**
		 * Check the input values for sanity */
		if (cutoff_min > cutoff_max) {
			std::cout << "Error in 'DSTViewer::returnClusterColor': cutoff_min < cutoff_max" << std::endl;
		}
		if (eneCluster < 0.0) {
			std::cout << "Error in 'DSTViewer::returnClusterColor': eneCluster is negative!" << std::endl;
		}
		if (cutoff_min < 0.0) {
			std::cout << "Error in 'DSTViewer::returnClusterColor': eneCluster is negative!" << std::endl;
		}

		int color = 0x000000; //default colour: black

		// Input values in log-scale
		float log_ene = std::log(eneCluster+1);
		float log_min = std::log(cutoff_min+1);
		float log_max = std::log(cutoff_max+1);

		int color_steps = 16*16; // 16*16 different steps from 0x600000 to 0x6000FF
		float log_delta = log_max - log_min;
		float log_step = log_delta/color_steps;

		int color_delta = (int) ((log_ene-log_min)/log_step); // which colour bin does the value go to? We have [0,16*16] bins

		//int color_delta_2 = (int)color_delta*16*16/2;
		//int color_delta_3 = (int)color_delta_2*16*16;

		if (color_delta >= 16*16){
			color = 0x990000;
		}
		else if (color_delta < 0){
			color = 0x9900FF;
		}
		else {
			color = 0x9900FF - color_delta;// + color_delta_2 - color_delta_3;
		}	

		//		// debug
		//		std::cout << "DEBUG: DSTViewer::returnClusterColor()" << std::endl;
		//		std::cout << "log_ene = " << log_ene << std::endl;
		//		std::cout << "log_min = " << log_min << std::endl;
		//		std::cout << "log_max = " << log_max << std::endl;
		//		std::cout << "log_step = " << log_step << std::endl;
		//		std::cout << "color_delta = " << color_delta << std::endl;
		//		using std::hex;
		//      	std::cout << hex << color << std::endl;

		return color;
	}


	void DSTViewer::showLegendSpectrum(const unsigned int color_steps, char scale, int colorMap, float ene_min, float ene_max, unsigned int ticks){

		const unsigned int numberOfColours = 3;
		unsigned int** rgb_matrix = new unsigned int*[color_steps]; //legend colour matrix;

		for (unsigned int i=0; i<color_steps; ++i){
			rgb_matrix[i] = new unsigned int[numberOfColours];
			ColorMap::selectColorMap(colorMap)(rgb_matrix[i], (float)i, 0.0, (float)color_steps);
			//			std::cout << "----------------------" << std::endl;
			//			std::cout << "i = " << i << std::endl;
			//			std::cout << "red = " << rgb_matrix[i][0] << std::endl;
			//			std::cout << "green = " << rgb_matrix[i][1] << std::endl;
			//			std::cout << "blue = " << rgb_matrix[i][2] << std::endl;
		}
		ced_legend(ene_min, ene_max, color_steps, rgb_matrix, ticks, scale);
	}

	int DSTViewer::returnRGBClusterColor(float eneCluster, float cutoff_min, float cutoff_max, int color_steps, char scale, int colorMap){

		int color = 0x000000; //default colour: black
		int color_delta = 0; //colour step in the [0, color_steps] spectrum
		unsigned int rgb[] = {0, 0, 0}; //array of RGB to be returned as one 0x000000 HEX value

		/**
		 * Check the input values for sanity */
		if (cutoff_min > cutoff_max) {
			std::cout << "Error in 'DSTViewer::returnRGBClusterColor': cutoff_min < cutoff_max" << std::endl;
		}
		if (eneCluster < 0.0) {
			std::cout << "Error in 'DSTViewer::returnRGBClusterColor': eneCluster is negative!" << std::endl;
		}
		if (cutoff_min < 0.0) {
			std::cout << "Error in 'DSTViewer::returnRGBClusterColor': eneCluster is negative!" << std::endl;
		}
		if (colorMap < 0 || colorMap > 6) {
			std::cout << "Error in 'DSTViewer::returnRGBClusterColor': wrong colorMap param!" << std::endl;
		}

		/**
		 * Different color scales */
		switch(scale){
			case 'a': default: //log

				// Input values in log-scale
				float log_ene = std::log(eneCluster+1);
				float log_min = std::log(cutoff_min+1);
				float log_max = std::log(cutoff_max+1);

				float log_delta = log_max - log_min;
				float log_step = log_delta/(float)color_steps;

				// log scale color_delta increment
				color_delta = (int) ((log_ene-log_min)/log_step); // which colour bin does the value go to? We have [0x00,0xFF] bins

				//std::cout << "Log color scale choosen" << std::endl;
				break;

			case 'b': //linear

				color_delta = (int)((eneCluster - cutoff_min)/(cutoff_max - cutoff_min)*color_steps);

				//std::cout << "linear color scale choosen" << std::endl;;
				break;
		}

		/*
		 * Bins outside of the colour range */
		if (color_delta >= color_steps){
			color_delta = color_steps;
			//std::cout << "color_delta > color_steps" << std::endl;
		}
		if (color_delta < 0){
			color_delta = 0;
			//std::cout << "color_delta < 0" << std::endl;
		}

		ColorMap::selectColorMap(colorMap)(rgb, color_delta, 0, color_steps);
		//		// DEBUG
		//		std::cout << "color_delta = " << color_delta << std::endl;
		//		std::cout << "color_delta = " << color_delta << std::endl;
		//		std::cout << "red = " << rgb[0] << std::endl;
		//		std::cout << "green = " << rgb[1] << std::endl;
		//		std::cout << "blue = " << rgb[2] << std::endl;

		color = ColorMap::RGB2HEX(rgb[0],rgb[1],rgb[2]);

		return color;
	}

	/**
	 * PRELIM */
	int DSTViewer::returnTrackSize(float type){

		int size = 1; //default size

		if (type==2112)			size = 1; //positive hadron
		else if (type==-2112)	size = 1; //negative hadron

		return size;
	}

	int DSTViewer::returnClusterSize(float eneCluster, float cutoff_min, float cutoff_max){ 

		/**
		 * Check the input values for sanity */
		if (cutoff_min > cutoff_max) {
			std::cout << "Error in 'DSTViewer::returnClusterSize': cutoff_min < cutoff_max" << std::endl;
		}
		if (eneCluster < 0.0) {
			std::cout << "Error in 'DSTViewer::returnClusterSize': eneCluster is negative!" << std::endl;
		}
		if (cutoff_min < 0.0) {
			std::cout << "Error in 'DSTViewer::returnClusterSize': eneCluster is negative!" << std::endl;
		}

		int size = 0; //default size: zero

		// sizes
		int size_min = 10;
		int size_max = 120;

		// Input values in log-scale
		float log_ene = std::log(eneCluster+1);
		float log_min = std::log(cutoff_min+1);
		float log_max = std::log(cutoff_max+1);

		int size_steps = size_max - size_min; // default: 90 step sizes
		float log_delta = log_max - log_min;
		float log_step = log_delta/size_steps;

		int size_delta = (int) ((log_ene-log_min)/log_step); // which size bin does the value go to?

		if (size_delta >= size_steps){
			size = size_max;
		}
		else if (size_delta < size_min){
			size = size_min;
		}
		else {
			size = size_min + size_delta;
		}	

		/**
		 * Check the output */
		if (size <=0){
			std::cout << "Error in 'DSTViewer::returnClusterSize': return size is negative!" << std::endl;
		}

		//		std::cout << "DEBUG: DSTViewer::returnClusterSize()" << std::endl;
		//		std::cout << "log_ene = " << log_ene << std::endl;
		//		std::cout << "log_min = " << log_min << std::endl;
		//		std::cout << "log_max = " << log_max << std::endl;
		//		std::cout << "log_step = " << log_step << std::endl;
		//		std::cout << "size_delta = " << size_delta << std::endl;
		//		std::cout << size << std::endl;

		return size;
	}
	
	int DSTViewer::returnJetLayer(std::string jetColName){
		
		int layer = BACKUP_LAYER;

		if (jetColName == "Durham_2Jets"){
			layer = JET2_LAYER;
		}
		else if (jetColName == "Durham_3Jets"){
			layer = JET3_LAYER;
		}
		else if (jetColName == "Durham_4Jets"){
			layer = JET4_LAYER;
		}
		else if (jetColName == "Durham_5Jets"){
			layer = JET5_LAYER;
		}
		else if (jetColName == "Durham_6Jets"){
			layer = JET6_LAYER;
		}
		else {
			layer = BACKUP_LAYER;
		}
	
		return layer;		
	}
	
//	float * DSTViewer::returnConeColor(std::string jetColName){
//		float RGBAcolor[4] = {0.4+0.1*i, 0.2+0.2*i, 1.0-0.25*i, 0.3};
//		
//		float opacity = 0.3;
//
//		if (jetColName == "Durham_2Jets"){
//			RGBAcolor[4] = {1, 0.1, 0.0, opacity};
//		}
//		else if (jetColName == "Durham_3Jets"){
//			RGBAcolor[4] = {1, 0.1, 0.0, opacity};
//		}
//		else if (jetColName == "Durham_4Jets"){
//			RGBAcolor[4] = {1, 0.1, 0.0, opacity};	
//		}
//		else if (jetColName == "Durham_5Jets"){
//			RGBAcolor[4] = {1, 0.1, 0.0, opacity};
//		}
//		else if (jetColName == "Durham_6Jets"){
//			RGBAcolor[4] = {1, 0.1, 0.0, opacity};
//		}
//		else {
//			RGBAcolor[4] = {1, 0.1, 0.0, opacity};		
//		}
//		
//		return RGBAcolor;
//	}
	
	
	int DSTViewer::returnJetColor(std::string jetColName, int colNumber){
		int color = 0x00000000;
		
		int white = 0x55555555;
		int black = 0x55000000;
		int red = 0x55550000;
		int blue = 0x55000055;
		int green = 0x55008000;
		int yellow = 0x55555500;
		int orange = 0x5555a500;
		int violet = 0x55ee82ee;
		int purple = 0x55800080;
		int silver = 0x55c0c0c0;
		int gold = 0x5555d700;
		int gray = 0x55808080;
		int aqua = 0x55005555;
		int skyblue = 0x5587ceeb;
		int lightblue = 0x55a558e6;
		int fuchsia = 0x55550055;
		int khaki = 0x55f0e68c;

	
		if (jetColName == "Durham_2Jets"){
			if (colNumber == 0){
				color = yellow;
			}
			if (colNumber == 1){
				color = red;
			}
		}
		else if (jetColName == "Durham_3Jets"){
			if (colNumber == 0){
				color = yellow;
			}
			if (colNumber == 1){
				color = red;
			}
			if (colNumber == 2){
				color = white;
			}
		}
		else if (jetColName == "Durham_4Jets"){
			if (colNumber == 0){
				color = yellow;
			}
			if (colNumber == 1){
				color = red;
			}
			if (colNumber == 2){
				color = white;
			}
			if (colNumber == 3){
				color = fuchsia;
			}
		}
		else if (jetColName == "Durham_5Jets"){
			if (colNumber == 0){
				color = yellow;
			}
			if (colNumber == 1){
				color = red;
			}
			if (colNumber == 2){
				color = white;
			}
			if (colNumber == 3){
				color = fuchsia;
			}
			if (colNumber == 4){
				color = black;
			}
		}
		else if (jetColName == "Durham_6Jets"){
			if (colNumber == 0){
				color = yellow;
			}
			if (colNumber == 1){
				color = red;
			}
			if (colNumber == 2){
				color = white;
			}
			if (colNumber == 3){
				color = fuchsia;
			}
			if (colNumber == 4){
				color = black;
			}
			if (colNumber == 5){
				color = green;
			}
		}
	
		return color;	
	}

#endif