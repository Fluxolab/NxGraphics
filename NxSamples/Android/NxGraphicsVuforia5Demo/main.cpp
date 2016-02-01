
/*===============================================================================================
 NxGraphics Engine Example
 Copyright (c), Perspective[S] Technologies 2014.

 This example shows how to basically instance the engine
===============================================================================================*/

 

#include <android/sensor.h>

#include <android/log.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <android/api-level.h>
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/TrackableResult.h>
#include <QCAR/Tool.h>
#include <QCAR/Tracker.h>
#include <QCAR/TrackerManager.h>
//#include <QCAR5/ImageTracker.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/DataSet.h>
#include <QCAR/VideoBackgroundTextureInfo.h>

#include "VuforiaMath.h"
#include "Shaders.h"

#define  LOG_TAG    "JNI"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace Nx;


	
#define logMsg( txt )NxLog::getSingleton().LogMessage( txt )  

NxGraphics * NxGraph = NULL;
Nx3D_Scene * Scene3DPlayer;
bool isReady = false;
NxEntity * NxBoxLeft = NULL;
 
NxNode * ModelNode = NULL;
NxEntity * NxModel = NULL;
 
NxNode * TheaterNode = NULL;
NxEntity * NxTheaterModel = NULL;
 
NxNode * ScreenNode = NULL;
NxEntity * NxScreenModel = NULL;

float mThresholdDistance = 1.0f; // is set later in function


const int numBoxes = 1;
std::vector<NxEntity * > mBoxes;
std::vector<NxNode * > mBoxesNodes;
std::vector<int> mBoxesRotValues;
std::vector<float> mBoxesDirection;

NxEntity * NxPlaneVideo = NULL;
NxNode * PlaneNode = NULL;
NxNode * StaticNode = NULL;
NxNode * DynamicNode = NULL;

NxRectangle2D * mBackgroundRectangle = NULL;

//background material
MaterialNx * mBackGroundMaterial = 0;
NxTechnique * mBackGroundTechnique = 0;
NxPass * mBackGroundPass = 0;
NxTextureUnit * mBackGroundUnit = 0; // white

NxTextureUnit * mFullScreenPhotoUnit = 0; // photo

NxPass * mFullScreenPhotoPass = 0;



AAssetManager* assetMgr = 0;
EGLContext context;

MaterialNx * MatVideo = NULL;
NxTextureUnit * MatVideoUnit0 =  NULL;
NxTextureUnit * MatVideoUnit1 = NULL; 
NxTextureUnit * MatVideoUnit2 = NULL;



NxPass * MatLeftPass1 = NULL;

NxPass * MatVideoPass = NULL;
int widthVideo = 512;
int widthBox= 20;


int mOgreScreenWidth = 0;
int mOgreScreenHeight = 0;
int mScreenWidth = 0;
int mScreenHeight = 0;


 
bool calibrateTrackingDone = false;
float mOffsetRotationY = 0.0f;


NxTextureUnit * mScreenTextureUnit = NULL;


////////////
#define LOOPER_ID 1
#define SAMP_PER_SEC 100

ASensorEventQueue* sensorEventQueue;

int accCounter = 0;
int64_t lastAccTime = 0;

int gyroCounter = 0;
int64_t lastGyroTime = 0;

int magCounter = 0;
int64_t lastMagTime = 0;

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/HelloJni/HelloJni.java
 */

static int get_sensor_events(int fd, int events, void* data);


static void   getQuaternionFromVector( float * Q , float * rv) {
        float w = (float) sqrtf(1 - rv[0]*rv[0] - rv[1]*rv[1] - rv[2]*rv[2]);
        //In this case, the w component of the quaternion is known to be a positive number
        
        Q[0] = w;
        Q[1] = rv[0];
        Q[2] = rv[1];
        Q[3] = rv[2];
    }


/////////////

static JavaVM* gVM = NULL;


#define ASENSOR_TYPE_ROTATION_VECTOR     11



/*
static int64_t time_stamp=-1;
static int64_t gyro_time_stamp=-1;
static float gyr_x=999,gyr_y=999,gyr_z=999; 
static float EPSILON = 0.000000001;
static float N2S = 1.0/1000000000.0;
static Nx::Quaternion deltaGyroQuaternion(  0,0,0,0);


void multiplyQuat(Nx::Quaternion* q1, Nx::Quaternion* q2){
    float nx = (q1->w)*(q2->x) + (q1->x)*(q2->w) + (q1->y)*(q2->z) - (q1->z)*(q2->y);
    float ny = (q1->w*q2->y - q1->x*q2->z + q1->y*q2->w + q1->z*q2->x);
    float nz = (q1->w*q2->z + q1->x*q2->y - q1->y*q2->x + q1->z*q2->w);
    float nw = (q1->w*q2->w - q1->x*q2->x - q1->y*q2->y - q1->z*q2->z);
    q1->x = nx;
    q1->y = ny;
    q1->z = nz;
    q1->w = nw;
}

*/


 Nx::Quaternion  mGyroscopeOrientation;
 float mGyroscopeOrientationYOsset =  -177;
 

JNIEXPORT jfloat JNICALL Java_com_hotstuff_main_OgreActivityJNI_GetGyroscopeY( JNIEnv * env, jobject obj ) {
	 
	 return (jfloat) mGyroscopeOrientation.getRoll().valueDegrees();
}
 
JNIEXPORT jfloat JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetGyroscopeOffset( JNIEnv * env, jobject obj, jfloat value ) {

	mGyroscopeOrientationYOsset = value;
	calibrateTrackingDone = false;
}
 
// from http://stackoverflow.com/questions/8989686/access-faster-polling-accelerometer-via-nativeactivity-ndk
static int get_sensor_events(int fd, int events, void* data) {

	Nx3DSceneDefault  * ptr = (Nx3DSceneDefault*) data;
	ASensorEvent event;
	while( ASensorEventQueue_getEvents( sensorEventQueue, &event, 1) > 0 ) {
		
        if( event.type == ASENSOR_TYPE_ACCELEROMETER ) {
               
        }
        else if( event.type == ASENSOR_TYPE_GYROSCOPE ) {
     
        }
        else if( event.type == ASENSOR_TYPE_MAGNETIC_FIELD ) {
 
               
        }
        else if( event.type == ASENSOR_TYPE_ROTATION_VECTOR ) {

            float q[4];
            getQuaternionFromVector(q, event.data );
            Nx::Quaternion  ori( q[0], -q[2],  q[1],  q[3]);
			
			mGyroscopeOrientation = ori;
			
            Nx::Quaternion x = Nx::Quaternion( Nx::Degree( -90 ), Nx::Vector3::UNIT_X); //  -90 landscape mode
 
			/*
			LOGD( "rotation YAW %f : " , ori.getYaw().valueDegrees()  ); //  
			LOGD( "rotation PITCH %f : " , ori.getPitch().valueDegrees()  ); 
			LOGD( "rotation ROLL %f : " , ori.getRoll().valueDegrees()  ); */
		 
  			if( !calibrateTrackingDone ) { 
				mOffsetRotationY = ((-177 - mGyroscopeOrientationYOsset) );
				calibrateTrackingDone = true;
			}
 
            Nx::Quaternion z = Nx::Quaternion( Nx::Degree(  mOffsetRotationY   )  , Nx::Vector3::UNIT_Z);  
			Nx::Quaternion mGyroscopeOrientation =  x * z * ori;
			
            ptr->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetOrientation(  mGyroscopeOrientation );	// x * z * ori

        }

		  

	}
	//should return 1 to continue receiving callbacks, or 0 to unregister
	return 1;
}


bool fadePhoto = false;
float fadePhototimeCurrent = 0.0f;
float fadePhotoTotalTimeFadeIn =  0.5f; // 5 seconds
float fadePhotoTotalTimeStay = 0.7f;


bool fadeWhite = false;
float fadeWhitetimeCurrent = 0.0f;
float fadeWhiteTotalTimeFadeIn = 0.4f; // 5 seconds
float fadeWhiteTotalTimeStay = 2.0f;


bool fadeEnd = false;
float fadeEndtimeCurrent = 0.0f;
float fadeEndTotalTimeFadeIn = 0.4f; // 5 seconds



bool canShowSecondRoom = false;

bool launchEndingTimer = false;
float launchEndingTimertimeCurrent = 0.0f;

bool fadeFinal = false;
float fadeFinalCurrent = 0.0f;

std::vector< Nx::Vector2 > mImagePos ;

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_StartFade(JNIEnv *env, jobject obj) 
{
	
	
	fadePhoto = true;
	
}



JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) 
{
	gVM = vm;
	return JNI_VERSION_1_4;
}


class FrameListener : public NxFrameListener  
{
	
public:
	bool frameStarted(const NxFrameEvent& evt) { 
 
		return true;
	}

	bool frameRenderingQueued(const NxFrameEvent& evt) {
		
		
		
		if( fadePhoto ) { 
		
			if( mFullScreenPhotoPass ) { 

				fadePhototimeCurrent += evt.timeSinceLastFrame;
				if( fadePhototimeCurrent <= fadePhotoTotalTimeFadeIn ) { 
					float fadeValue = (1.0f * fadePhototimeCurrent) / fadePhotoTotalTimeFadeIn;
					
					//mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", fadeValue);	

					
					
					mBackGroundPass->SetFragmentParameterValue( "alpha", fadeValue);						
				} 
				
				if( fadePhototimeCurrent >= (fadePhotoTotalTimeFadeIn + fadePhotoTotalTimeStay)) { 
					mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", 0.0f );
					mBackGroundPass->SetFragmentParameterValue( "alpha", 1.0f);	
					fadePhoto = false;
					fadeWhite = true;
					canShowSecondRoom = true;
				}
			}
 
		}		
 
		if( fadeWhite ) { 
			if( mBackGroundPass ) { 
			
				 fadeWhitetimeCurrent += evt.timeSinceLastFrame;
				if( fadeWhitetimeCurrent <= fadeWhiteTotalTimeFadeIn ) { 
					float fadeValue = (1.0f * fadeWhitetimeCurrent) / fadeWhiteTotalTimeFadeIn;
					
					
					//fade white out
					//mBackGroundPass->SetFragmentParameterValue( "alpha", 1.0f - fadeValue ); 
					// fade photo in
					mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", fadeValue );
					//mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", fadeValue);	

					//mBackGroundPass->SetFragmentParameterValue( "alpha", fadeValue);						
				} 
				
				if( fadeWhitetimeCurrent >= (fadeWhiteTotalTimeFadeIn + fadeWhiteTotalTimeStay)) { 
						mBackGroundPass->SetFragmentParameterValue( "alpha", 0.0f ); 
						mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", 1.0f);
						 fadeWhite = false;
						 fadeEnd = true; 
				}
			
			
			
			
			/*
			
				float fadeValue = (1.0f * fadeWhitetimeCurrent) / fadeWhiteTotalTimeFadeIn;
				
				//fade white out
				mBackGroundPass->SetFragmentParameterValue( "alpha", 1.0f - fadeValue ); 
				// fade photo in
				mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", fadeValue );	
				
				fadeWhitetimeCurrent += evt.timeSinceLastFrame;
				if( fadeWhitetimeCurrent >= fadeWhiteTotalTimeFadeIn ) { 
				
					mBackGroundPass->SetFragmentParameterValue( "alpha", 0.0f ); 
						mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", 1.0f);
						 fadeWhite = false;
						 fadeEnd = true; 
				}
				
				*/
				 
				
				
			}
		}
 
		if( fadeEnd ) { 
			 
			float fadeValue = (1.0f * fadeEndtimeCurrent) / fadeEndTotalTimeFadeIn;
			mBackGroundPass->SetFragmentParameterValue( "alpha", 0.0f );
			mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", 1.0f - fadeValue );
			fadeEndtimeCurrent += evt.timeSinceLastFrame;
			if( fadeEndtimeCurrent >= fadeEndTotalTimeFadeIn ) { 
			
			
				mBackGroundPass->SetFragmentParameterValue( "alpha", 0.0f );
			mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", 0.0f );
			
			
				 fadeEnd = false;
			}
			 
		}	
 
		if(launchEndingTimer) { 
	 
			launchEndingTimertimeCurrent += evt.timeSinceLastFrame;
			if( launchEndingTimertimeCurrent >= 30.0f ) { 
				launchEndingTimer = false;
				fadeFinal = true;
				NxTextureManager::getSingleton().LoadTexture( "black.jpg", "Popular" ) ;
				mBackGroundUnit->SetTextureName("black.jpg");  
				mBackGroundPass->SetFragmentParameterValue( "alpha", 0.0f ); 
			}
	 
		}
		
		if( fadeFinal ) { 
		
				float fadeValue = (1.0f * fadeFinalCurrent) / 5.0f;
				mBackGroundPass->SetFragmentParameterValue( "alpha", fadeValue );
				fadeFinalCurrent += evt.timeSinceLastFrame;
				if( fadeFinalCurrent >= 5.0f ) { 
					 fadeFinal = false; 
				}

		}
	
		return true;
	}

    bool frameEnded(const NxFrameEvent& evt) { 
			 return true;
			 
	}	
	
};

class EngineListener : public NxEngineListener
{
public :
	EngineListener(){}

	~EngineListener(){}

	void OnWindowMoved( unsigned int PosX, unsigned int PosY ){
		LOGD( "OnWindowMoved %d X %d : " , PosX, PosY );
	}

	void OnWindowResized( unsigned int Width , unsigned int Height ) {
		LOGD( "OnWindowResized %d X %d : " , Width, Height );
	}

	bool OnWindowClosing() {
		LOGD( "OnWindowClosing" );
		return true;
	}
	void OnWindowClosed() {
		LOGD( "OnWindowClosed" );
	}

	void OnWindowFocusChange(){
		LOGD( "OnWindowFocusChange" );
	}
};

class NxInputCallBack : public NxInputListener
{
public :
	 NxInputCallBack(){ }

	~NxInputCallBack(){ }

	void OnKeyPressed( const NxKeyCode &e ) {
		LOGD("OnKeyPressed");

		if(  NXKC_ESCAPE == e ){
			 
		}

	}

	void OnKeyReleased( const NxKeyCode &e ){
		LOGD("OnKeyReleased");
	}

	void OnMouseMoved( const OIS::MouseEvent &e ){
		LOGD("OnKeyMouseMoved");
	}

	void OnMouseButtonPressed( const OIS::MouseEvent &e, NxMouseButtonID id ){
		LOGD("OnKeyMousePressed");
	}

	void OnMouseButtonReleased( const OIS::MouseEvent &e, NxMouseButtonID id ){
		LOGD("OnKeyMouseReleased");
	}

	void OnJoystickButtonPressed( const OIS::JoyStickEvent &arg, int button ){
		LOGD("OnJoystickButtonPressed");
	}

	void OnJoystickButtonReleased( const OIS::JoyStickEvent &arg, int button ){
		LOGD("OnJoystickButtonReleased");
	}

	void OnJoystickAxisMoved( const OIS::JoyStickEvent &arg, int axis ){
		LOGD("OnJoystickAxisMoved");
	}
};



JNIEXPORT jlong JNICALL Java_com_hotstuff_main_OgreActivityJNI_GetEngineContext() {
	 
	return (long)&context;
}
 
JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_CreateEngine(JNIEnv *env, jobject obj, jobject surface, jobject assetManager, jstring splashName ) 
{
 
	mImagePos.push_back( Nx::Vector2( 3767 , 700 ) );
	mImagePos.push_back( Nx::Vector2( 3767 , 610 ) );
	mImagePos.push_back( Nx::Vector2( 3767 , 526 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , 440 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , 351 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , 267 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , 178 ) );	
	
	
	mImagePos.push_back( Nx::Vector2( 3914 , 700 ) );
	mImagePos.push_back( Nx::Vector2( 3914 , 610 ) );
	mImagePos.push_back( Nx::Vector2( 3914 , 526 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , 440 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , 351 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , 267 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , 178 ) );	


	mImagePos.push_back( Nx::Vector2( 4119 , 700 ) );
	mImagePos.push_back( Nx::Vector2( 4119 , 610 ) );
	mImagePos.push_back( Nx::Vector2( 4119 , 526 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , 440 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , 351 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , 267 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , 178 ) );	
	
	
	mImagePos.push_back( Nx::Vector2( 4325 , 700 ) );
	mImagePos.push_back( Nx::Vector2( 4325 , 610 ) );
	mImagePos.push_back( Nx::Vector2( 4325 , 526 ) );	
	mImagePos.push_back( Nx::Vector2( 4325 , 440 ) );	
	mImagePos.push_back( Nx::Vector2( 4325 , 351 ) );	
	mImagePos.push_back( Nx::Vector2( 4325 , 267 ) );	
	mImagePos.push_back( Nx::Vector2( 4325 , 178 ) );		
	
	
	
	// right
	
	mImagePos.push_back( Nx::Vector2( 3767 , -197 ) );
	mImagePos.push_back( Nx::Vector2( 3767 , -282 ) );
	mImagePos.push_back( Nx::Vector2( 3767 , -367 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , -452 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , -537 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , -622 ) );	
	mImagePos.push_back( Nx::Vector2( 3767 , -707 ) );	
	
	mImagePos.push_back( Nx::Vector2( 3914 , -197 ) );
	mImagePos.push_back( Nx::Vector2( 3914 , -282 ) );
	mImagePos.push_back( Nx::Vector2( 3914 , -367 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , -452 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , -537 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , -622 ) );	
	mImagePos.push_back( Nx::Vector2( 3914 , -707 ) );	
	
	mImagePos.push_back( Nx::Vector2( 4119 , -197 ) );
	mImagePos.push_back( Nx::Vector2( 4119 , -282 ) );
	mImagePos.push_back( Nx::Vector2( 4119 , -367 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , -452 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , -537 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , -622 ) );	
	mImagePos.push_back( Nx::Vector2( 4119 , -707 ) );		
	
	
	mImagePos.push_back( Nx::Vector2( 4331 , -197 ) );
	mImagePos.push_back( Nx::Vector2( 4331 , -282 ) );
	mImagePos.push_back( Nx::Vector2( 4331 , -367 ) );	
	mImagePos.push_back( Nx::Vector2( 4331 , -452 ) );	
	mImagePos.push_back( Nx::Vector2( 4331 , -537 ) );	
	mImagePos.push_back( Nx::Vector2( 4331 , -622 ) );	
	mImagePos.push_back( Nx::Vector2( 4331 , -707 ) );	

// 3 middles
mImagePos.push_back( Nx::Vector2( 4331 ,  93 ) );		
mImagePos.push_back( Nx::Vector2( 4331 ,  2 ) );	
mImagePos.push_back( Nx::Vector2( 4331 ,  -93 ) );	
	
	
	
 
	LOGD("----> NDK CreateEngine CALLED !!! ");

	if( NxGraph != NULL ){  LOGD("----> NDK CreateEngine already done !!! "); return;}

	assetMgr = AAssetManager_fromJava( env, assetManager );
	ANativeWindow* nativeWnd = ANativeWindow_fromSurface(env, surface);

	mOgreScreenWidth = ANativeWindow_getWidth( nativeWnd );
	mOgreScreenHeight = ANativeWindow_getHeight( nativeWnd );

	//Initialize the engine
	NxGraphicsDesc EngineDesc; 
	EngineDesc.Vsync = true ;
	EngineDesc.Renderer = NxGraphics_GLES2; 
	EngineDesc.mAssetMgr = assetMgr;
	EngineDesc.mNativeWnd = nativeWnd;
 
	float screenRatio = (float)mOgreScreenWidth/(float)mOgreScreenHeight;

/*
	EngineDesc.mSplashSize[0] = 0.25f;
	EngineDesc.mSplashSize[1] = 0.0f;
	EngineDesc.mSplashSize[2] = 0.5f;
	EngineDesc.mSplashSize[3] = 0.9f;*/
    
    EngineDesc.mSplashSize[0] = 0.0f;
	EngineDesc.mSplashSize[1] = 0.0f;
	EngineDesc.mSplashSize[2] = 1.0f;
	EngineDesc.mSplashSize[3] = 1.0f;
	
	// get splash file
	const char *splashNameJni = env->GetStringUTFChars( splashName, NULL );
	EngineDesc.mSplashTexture = splashNameJni;
	env->ReleaseStringUTFChars( splashName, splashNameJni );

	//LOGD("----> mNativeWnd : %d ", (int)EngineDesc.mNativeWnd  );
	//LOGD("----> EngineDesc.GlContext : %d ", (int)EngineDesc.GlContext  );

	NxGraph = new NxGraphics();
	NxGraph->CreateNxEngine( EngineDesc );

	NxDeviceManager * DeviceMgr = NxDeviceManager::getSingletonPtr(); 
	NxSoundManager  * SoundMgr  = NxSoundManager::getSingletonPtr();
	NxMocapManager  * MocapMgr  = NxMocapManager::getSingletonPtr();
	
	

	
	
	
	
  
	Nx3DSceneDesc Desc;
    Desc.mType = Nx3D_Scene_Default;
	Desc.mRenderWindow = NxEngine::getSingleton().GetNxWindow();
	Scene3DPlayer = NxScene3DManager::getSingleton().CreateNx3DScene( Desc );
    Scene3DPlayer->SetViewportColour( NxColourValue(0,0,1,1) ); 
	Desc.mRenderWindow->GetCustomAttribute( "GLCONTEXT", &context  );
	LOGD("----> GLCONTEXT id : %d ", (int)context );

	ModelNode = Scene3DPlayer->CreateNxNode( "TheaterNode" + NxVideoUtils::ToString( 0 ) );

	NxModel = ModelNode->CreateNxEntity( "room1.mesh" );
	NxModel->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxModel->SetScale( Nx::Vector3(1,1,1)); 

	NxEntity * NxChairsRoom1BackLeft = ModelNode->CreateNxEntity( "chairs_room1_back_left.mesh" );
	NxChairsRoom1BackLeft->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom1BackLeft->SetScale( Nx::Vector3(1,1,1)); 	

	NxEntity * NxChairsRoom1BackRight = ModelNode->CreateNxEntity( "chairs_room1_back_right.mesh" );
	NxChairsRoom1BackRight->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom1BackRight->SetScale( Nx::Vector3(1,1,1)); 		

	NxEntity * NxChairsRoom1FrontLeft = ModelNode->CreateNxEntity( "chairs_room1_front_left.mesh" );
	NxChairsRoom1FrontLeft->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom1FrontLeft->SetScale( Nx::Vector3(1,1,1)); 		

	NxEntity * NxChairsRoom1FrontRight = ModelNode->CreateNxEntity( "chairs_room1_front_right.mesh" );
	NxChairsRoom1FrontRight->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom1FrontRight->SetScale( Nx::Vector3(1,1,1)); 		

	NxEntity * NxRoom2 = ModelNode->CreateNxEntity( "room2.mesh" );
	NxRoom2->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxRoom2->SetScale( Nx::Vector3(1,1,1)); 

	NxEntity * NxChairsRoom2BackLeft = ModelNode->CreateNxEntity( "chairs_room2_back_left.mesh" );
	NxChairsRoom2BackLeft->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom2BackLeft->SetScale( Nx::Vector3(1,1,1)); 	

	NxEntity * NxChairsRoom2BackRight = ModelNode->CreateNxEntity( "chairs_room2_back_right.mesh" );
	NxChairsRoom2BackRight->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom2BackRight->SetScale( Nx::Vector3(1,1,1)); 	

	NxEntity * NxChairsRoom2FrontLeft = ModelNode->CreateNxEntity( "chairs_room2_front_left.mesh" );
	NxChairsRoom2FrontLeft->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom2FrontLeft->SetScale( Nx::Vector3(1,1,1)); 		

	NxEntity * NxChairsRoom2FrontRight = ModelNode->CreateNxEntity( "chairs_room2_front_right.mesh" );
	NxChairsRoom2FrontRight->SetPosition( Nx::Vector3( 0, 0, 0)); 	
	NxChairsRoom2FrontRight->SetScale( Nx::Vector3(1,1,1)); 		

	 
	ScreenNode = Scene3DPlayer->CreateNxNode( "ScreenNode" + NxVideoUtils::ToString( 0 ) );
	NxScreenModel = ScreenNode->CreateNxEntity( "screen.mesh" );
	NxScreenModel->SetPosition( Nx::Vector3( 0 , 0 , 0)); 	
	NxScreenModel->SetScale( Nx::Vector3( 1,1,1));   
 
  
    
   // create screen material for the picture later

	 
	
	// done

  
    
    
    
    
    
    
    
    
 
        
        
        
        
    
     
	 
	 
	 /*
	MaterialNx * MatLeft =  NxMaterialManager::getSingleton().CreateMaterial("BoxesMaterial");
	NxTechnique * MatLeftTechnique = MatLeft->CreateTechnique("");
	NxPass * MatLeftPass =  MatLeftTechnique->CreatePass("");
	MatLeftPass->SetSceneBlending(  NXBLEND_TRANSPARENT_ALPHA ); //<<---- cest ca la merde opengles ne supporte pas glAlphaFunc et GL_ALPHA_TEST
	MatLeftPass->SetDepthCheckEnabled(true);
	MatLeftPass->SetDepthWriteEnabled(true);
	MatLeftPass->SetLightingEnabled(false);
	MatLeftPass->SetCullingMode(NXCULL_CLOCKWISE);
	NxTextureUnit * MatLeftUnit = MatLeftPass->CreateTextureUnit("");
	MatLeftUnit->SetTextureAddressingMode(TEXTURE_BORDER);
	MatLeftUnit->SetTextureName("NxLogo.jpg"); */

 
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetPosition( Nx::Vector3( 0,180, -200 ) );
	
	
//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetPosition(Nx::Vector3(0, 300, 0));
	//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->LookAt(Nx::Vector3(0,300,-2500.0f));
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetNearPlane(10.0f);
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetFarPlane(5000.0f);//500 ok
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetFieldOfView( 90.0f  );//GetFovDegrees() );	
	
	

	//double Distance = 500.0;
	//// Calculate FOV H in degrees from Camera Distance.
	//double heightscreen = 810.0f;//800.0f;
	//double FOVRad = 2.0 *  atan(( heightscreen / 2.0 ) / Distance );
	//double FOVDeg = FOVRad * 180.0 / Nx::Math::PI;

	//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetPosition( Nx::Vector3( 0,0, Distance ) );
	//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetAspectRatio(1440.0f/heightscreen);
	//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetFieldOfView( Nx::Degree( FOVDeg ).valueDegrees()  );
	  
	  
	// CREATE WHITE FULL SCREEN
	mBackGroundMaterial = NxMaterialManager::getSingleton().CreateMaterial("whiteMaterial");
	mBackGroundTechnique = mBackGroundMaterial->CreateTechnique("");
	mBackGroundPass =  mBackGroundTechnique->CreatePass("");
 
	mBackGroundPass->SetSceneBlending( NXBLEND_TRANSPARENT_ALPHA ); 
	mBackGroundPass->SetDepthCheckEnabled(true);
	mBackGroundPass->SetDepthWriteEnabled(true);
	mBackGroundPass->SetLightingEnabled(false);
	
	
	 
 
	mBackGroundPass->SetCullingMode( NXCULL_CLOCKWISE );  
	mBackGroundUnit = mBackGroundPass->CreateTextureUnit("");
	NxTextureManager::getSingleton().LoadTexture( "white.jpg", "Popular" ) ;
	mBackGroundUnit->SetTextureName("white.jpg");  
	mBackGroundUnit->SetTextureAddressingMode(TEXTURE_CLAMP);
	 
	
	
	const char * strVert = 
	"#version 100\n"
	"precision mediump int;"
	"precision mediump float;"
	"uniform mat4 worldViewProj;"
	"attribute vec4 vertex;"
	"attribute vec2 uv0;"
	"varying vec2 uv;"
	"void main()"
	"{"
	"	gl_Position = worldViewProj * vertex;"
	"	uv = uv0;"
	"}"	;
	
	const char * strFrag = 
	"precision mediump float;"
	"uniform sampler2D tex0;"
	"uniform float alpha;"
	"varying vec2 uv;"
	"void main()"
	"{"  
	"vec2 uv2 = vec2(uv.s, 1.0 - uv.t);"
	"vec4 normalColor = texture2D(tex0, uv2);"
	"gl_FragColor = vec4( normalColor.r  , normalColor.g  , normalColor.b  , alpha);"
	"}";


	//create 2 shaders
	NxVertexShader * vert = NxMaterialManager::getSingleton().CreateVertexProgram( "whitevert", "glsles" );
	NxPixelShader * frag = NxMaterialManager::getSingleton().CreateFragmentProgram( "whitefrag", "glsles" ); 
	vert->SetSource(  strVert );
	vert->Load();
	frag->SetSource(  strFrag );
	frag->Load();
	
	mBackGroundPass->SetVertexProgram(vert->GetName());
	mBackGroundPass->SetFragmentProgram(frag->GetName() ); 
	mBackGroundPass->SetVertexAutoParameterValue( "worldViewProj", ACT_WORLDVIEWPROJ_MATRIX ); 
	mBackGroundPass->SetFragmentParameterValue( "tex0", 0 );
	mBackGroundPass->SetFragmentParameterValue( "alpha", (float)0.0f );	
 
	NxNode * BackgroundNode = Scene3DPlayer->CreateNxNode( "whiteNode" );
	mBackgroundRectangle = BackgroundNode->CreateNxRectangle2D( "whiteRectangle", true );
	mBackgroundRectangle->SetMaterialName( mBackGroundMaterial->GetName() );
	mBackgroundRectangle->SetRenderQueueGroup( 100 );
	
	
 
	
	
  
	//////////////////////////////////////////////////////////////////////////
	
    ASensorEvent event;
    int events, ident;
    ASensorManager* sensorManager;
    const ASensor* accSensor;
    const ASensor* gyroSensor;
    const ASensor* magSensor;
	const ASensor* rotVector;
    void* sensor_data = malloc(1000);
 
    ALooper* looper = ALooper_forThread();

    if( looper == NULL ) {
        looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    }

    sensorManager = ASensorManager_getInstance();

    accSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    gyroSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GYROSCOPE);
    magSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);    
    rotVector = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ROTATION_VECTOR);
 
    sensorEventQueue = ASensorManager_createEventQueue(sensorManager, looper, 4, get_sensor_events, Scene3DPlayer);

    ASensorEventQueue_enableSensor(sensorEventQueue, accSensor);
    ASensorEventQueue_enableSensor(sensorEventQueue, gyroSensor);
    ASensorEventQueue_enableSensor(sensorEventQueue, magSensor);
    ASensorEventQueue_enableSensor(sensorEventQueue, rotVector);
 
    //Sampling rate: 100Hz
    int a = ASensor_getMinDelay(accSensor);
    int b = ASensor_getMinDelay(gyroSensor);
    int c = ASensor_getMinDelay(magSensor);
	int d = ASensor_getMinDelay(rotVector);
    //LOGI("min-delay: %d, %d, %d",a,b,c);
    ASensorEventQueue_setEventRate(sensorEventQueue, accSensor, 100000);
    ASensorEventQueue_setEventRate(sensorEventQueue, gyroSensor, 100000);
    ASensorEventQueue_setEventRate(sensorEventQueue, magSensor, 100000);
    ASensorEventQueue_setEventRate(sensorEventQueue, rotVector, 100000);
 
	
	//////////////////
	
	
 logMsg("=====>>>>>>>ATTTACHING "); 
	
	
	FrameListener * frameListener = new FrameListener();
	NxEngine::getSingleton().AddFrameListener( frameListener ) ;
 
	//NxGraph->GetEngineManager()->AddFrameListener( frameListener ) ;
 
	EngineListener * liste = new EngineListener();
	NxEngine::getSingleton().GetNxWindow()->AddListener(  liste );

	NxInputCallBack  * InputCallback; 
	NxEngine::getSingleton().GetNxWindow()->AddInputListener( InputCallback = new NxInputCallBack() ); 
 
	   
	    
 logMsg("=====>>>>>>>ATTTACHING  DONE"); 		
	
	
	


	isReady = true;
	 
	return;
}


JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_DeleteEngine(JNIEnv *env, jobject obj) 
{
 
	NxGraph->ReleaseEngine();

	delete NxGraph;
	NxGraph = 0;

	isReady = false;


	return;
}


JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetScreenImage(JNIEnv * env, jobject obj,  jstring imagePath  ) {


	const char *Stringp = env->GetStringUTFChars( imagePath, NULL );
	std::string videofilepath =  std::string( Stringp );
 
	// create photo material for shader
	NxTextureImage * tex =  NxTextureManager::getSingleton().CreateTextureImage( "photo" + NxVideoUtils::ToString( 0 ), videofilepath );
	MaterialNx * MatLeft1 =  NxMaterialManager::getSingleton().CreateMaterial("photomat" + NxVideoUtils::ToString( 0) );
	NxTechnique * MatLeftTechnique1 = MatLeft1->CreateTechnique("");
	MatLeftPass1 =  MatLeftTechnique1->CreatePass("");
	MatLeftPass1->SetSceneBlending(  NXBLEND_TRANSPARENT_ALPHA ); 
	MatLeftPass1->SetDepthCheckEnabled(true);
	MatLeftPass1->SetDepthWriteEnabled(true);
	MatLeftPass1->SetLightingEnabled(false);
	MatLeftPass1->SetCullingMode( NXCULL_CLOCKWISE ); 
	mScreenTextureUnit = MatLeftPass1->CreateTextureUnit("");
	mScreenTextureUnit->SetTextureAddressingMode(TEXTURE_CLAMP);

	////////
 
	const char * strVert = 
	"#version 100\n"
	"precision mediump int;"
	"precision mediump float;"
	"uniform mat4 worldViewProj;"
	"attribute vec4 vertex;"
	"attribute vec2 uv0;"
	"varying vec2 uv;"
	"void main()"
	"{"
	"	gl_Position = worldViewProj * vertex;"
	"	uv = uv0;"
	"}"	;
	
	const char * strFrag = 
	"precision mediump float;"
	"uniform sampler2D tex0;"
	"uniform float alpha;"
	"varying vec2 uv;"
	"void main()"
	"{"  
	"vec2 uv2 = vec2(uv.s, 1.0 - uv.t);"
	"vec4 normalColor = texture2D(tex0, uv2);"
	"float gray = 0.299*normalColor.r + 0.587*normalColor.g + 0.114*normalColor.b;"
	"gl_FragColor = vec4( gray*alpha + ( 1.0*(1.0-alpha) ), gray*alpha+ ( 1.0*(1.0-alpha) ), gray*alpha+ ( 1.0*(1.0-alpha) ),  1.0);"
	"}";


	//create 2 shaders
	NxVertexShader * vert = NxMaterialManager::getSingleton().CreateVertexProgram( "testvert", "glsles" );
	NxPixelShader * frag = NxMaterialManager::getSingleton().CreateFragmentProgram( "testfrag", "glsles" ); 
	vert->SetSource(  strVert );
	vert->Load();
	frag->SetSource(  strFrag );
	frag->Load();		
	MatLeftPass1->SetVertexProgram(vert->GetName());
	MatLeftPass1->SetFragmentProgram(frag->GetName() ); 
	mScreenTextureUnit->SetTextureName(  "photo" + NxVideoUtils::ToString( 0 )  );
	//mScreenTextureUnit->SetTextureScale( 1.0, -1.0f );	
	MatLeftPass1->SetVertexAutoParameterValue( "worldViewProj", ACT_WORLDVIEWPROJ_MATRIX ); 
	MatLeftPass1->SetFragmentParameterValue( "tex0", 0 );
	MatLeftPass1->SetFragmentParameterValue( "alpha", (float)0.0f );
	// assign material
	NxScreenModel->SetMaterialName("photomat" + NxVideoUtils::ToString( 0));
	
	
	 //////////////////////////////////////////////////////////////////////////////
	 
	  
	// CREATE PHOTO FULL SCREEN
	
	MaterialNx * mFullScreenPhotoMaterial = NxMaterialManager::getSingleton().CreateMaterial("photoMaterial");
	NxTechnique * mFullScreenPhotoTechnique = mFullScreenPhotoMaterial->CreateTechnique("");
	  mFullScreenPhotoPass =   mFullScreenPhotoTechnique->CreatePass("");
 
	mFullScreenPhotoPass->SetSceneBlending( NXBLEND_TRANSPARENT_ALPHA ); 
	mFullScreenPhotoPass->SetDepthCheckEnabled(true);
	mFullScreenPhotoPass->SetDepthWriteEnabled(true);
	mFullScreenPhotoPass->SetLightingEnabled(false);
 
	mFullScreenPhotoPass->SetCullingMode( NXCULL_CLOCKWISE );  
	mFullScreenPhotoUnit = mFullScreenPhotoPass->CreateTextureUnit("");
	mFullScreenPhotoUnit->SetTextureAddressingMode(TEXTURE_CLAMP); 
	
	
	const char * strVertFullPhoto = 
	"#version 100\n"
	"precision mediump int;"
	"precision mediump float;"
	"uniform mat4 worldViewProj;"
	"attribute vec4 vertex;"
	"attribute vec2 uv0;"
	"varying vec2 uv;"
	"void main()"
	"{"
	"	gl_Position = worldViewProj * vertex;"
	"	uv = uv0;"
	"}"	;
	
	const char * strFragFullPhoto = 
	"precision mediump float;"
	"uniform sampler2D tex0;"
	"uniform float alpha;"
	"varying vec2 uv;"
	"void main()"
	"{"  
	"vec2 uv2 = vec2(1.0 - uv.s, uv.t);"
	"vec4 normalColor = texture2D(tex0, uv2);"
	"float gray = 0.299*normalColor.r + 0.587*normalColor.g + 0.114*normalColor.b;"
	"gl_FragColor = vec4( gray*alpha  , gray*alpha , gray*alpha ,  alpha);"
	"}";
	
	
	

	NxNode * mFullScreenPhotoNode = Scene3DPlayer->CreateNxNode( "photoNode" );
	NxEntity * mFullScreenPhotoRectangle = mFullScreenPhotoNode->CreateNxRectangle2D( "photoRectangle", true );
	mFullScreenPhotoRectangle->SetMaterialName( mFullScreenPhotoMaterial->GetName() ); 
	
	
	NxVertexShader * vertPhoto = NxMaterialManager::getSingleton().CreateVertexProgram( "photoFullvert", "glsles" );
	NxPixelShader * fragPhoto = NxMaterialManager::getSingleton().CreateFragmentProgram( "photoFullfrag", "glsles" ); 
	vertPhoto->SetSource(  strVertFullPhoto );
	vertPhoto->Load();
	fragPhoto->SetSource(  strFragFullPhoto );
	fragPhoto->Load();		
	mFullScreenPhotoPass->SetVertexProgram(vertPhoto->GetName());
	mFullScreenPhotoPass->SetFragmentProgram(fragPhoto->GetName() ); 	
	mFullScreenPhotoUnit->SetTextureName( "photo" + NxVideoUtils::ToString( 0 )  );    
	mFullScreenPhotoRectangle->SetRenderQueueGroup( 101 );
	
	
	mFullScreenPhotoPass->SetVertexAutoParameterValue( "worldViewProj", ACT_WORLDVIEWPROJ_MATRIX ); 
	mFullScreenPhotoPass->SetFragmentParameterValue( "tex0", 0 );
	mFullScreenPhotoPass->SetFragmentParameterValue( "alpha", (float)0.0f );	
	

	
	//mFullScreenPhotoUnit->SetOpacity( 0.0f );  
	  
	/////////////////////////////////////////////////////////////////////
	
  
	env->ReleaseStringUTFChars( imagePath, Stringp );
	
	
	LOGD("=============>>> set image screen OK");

 
}
 


	 


JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetThreshHoldDistance(JNIEnv * env, jobject obj,  jfloat distance  ) {

	mThresholdDistance = distance;
 
}
 

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetViewportSize(JNIEnv * env, jobject obj,  jfloat Left, jfloat Top, jfloat Width, jfloat Height ) {

	NxEngine::getSingleton().GetNxViewport()->SetDimensions(  Left,  Top,  Width, Height );
 
}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetScreenSize(JNIEnv * env, jobject obj,  jint Width, jint Height ) {

	mScreenWidth = Width;
	mScreenHeight = Height;
}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetTextureTransform( JNIEnv * env, jobject obj, jfloatArray trans, jfloatArray rotate, jfloatArray scale ) {

	jfloat* fltrotate = env->GetFloatArrayElements( rotate,0);
	jfloat* fltscale  = env->GetFloatArrayElements( scale, 0);
	jfloat* flttrans  = env->GetFloatArrayElements( trans, 0);


	Nx::Matrix4 xform = Nx::Matrix4::IDENTITY;
	Nx::Quaternion qx, qy, qz, qfinal;
	qx.FromAngleAxis(Nx::Degree( fltrotate[0] ), Nx::Vector3::UNIT_X);
	qy.FromAngleAxis(Nx::Degree( fltrotate[1] ), Nx::Vector3::UNIT_Y);
	qz.FromAngleAxis(Nx::Degree( fltrotate[2] ), Nx::Vector3::UNIT_Z);
	qfinal = qx * qy * qz;

	Nx::Vector3 translate;
	translate.x = flttrans[0];
	translate.y = flttrans[1];
	translate.z = flttrans[2];

	Nx::Matrix3 rot3x3, scale3x3;
	qfinal.ToRotationMatrix(rot3x3);
	scale3x3 = Nx::Matrix3::ZERO;
	scale3x3[0][0] = fltscale[0];
	scale3x3[1][1] = fltscale[1];
	scale3x3[2][2] = fltscale[2];

	xform = rot3x3 * scale3x3;
	xform.setTrans(translate);

	mBackGroundUnit->SetTextureTransform( xform );

	env->ReleaseFloatArrayElements(trans,  flttrans, 0);
	env->ReleaseFloatArrayElements(rotate, fltrotate, 0);
	env->ReleaseFloatArrayElements(scale,  fltscale, 0);

}

#define PI 3.1415926f
JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetTextureFOV(JNIEnv * env, jobject obj, jdouble fovyRadians, jdouble fovRadians ) {

		//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetFieldOfView(  180 * fovyRadians / PI  );
}
 


JNIEXPORT jobjectArray JNICALL  Java_com_hotstuff_main_OgreActivityJNI_CopyFilesToPath( JNIEnv * env, jobject obj, jstring dstfilepath, jobject assetManager ) {

	jobjectArray ret;

	const char *Stringp = env->GetStringUTFChars(  dstfilepath, NULL );
	int StringLen = env-> GetStringUTFLength( dstfilepath );

	AAssetManager* pAssetManager = AAssetManager_fromJava( env, assetManager );

	std::vector<std::string> mFiles;

	std::string FolderPath = "VideoData"; 
	AAssetDir* assetDir = AAssetManager_openDir( pAssetManager , FolderPath.c_str());
	const char* filename = (const char*)NULL;
	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) { 
		std::string LocalFilePath =  FolderPath+"/"+std::string( filename );
		AAsset* asset = AAssetManager_open( pAssetManager, LocalFilePath.c_str(), AASSET_MODE_BUFFER);
		if( asset ){
		off_t length = AAsset_getLength(asset);
		char * membuf = new char[length];
		int nb_read = 0;
		std::string filepathStr = std::string( Stringp ) + "/"+std::string( filename );
		FILE* out = fopen( filepathStr.c_str(), "w");
		mFiles.push_back( filepathStr );
		while ((nb_read = AAsset_read(asset, membuf, length)) > 0)
			fwrite(membuf, nb_read, 1, out);

		fclose(out);
		delete membuf;
		AAsset_close(asset);
		}//if asset
	}
	AAssetDir_close(assetDir);
	env->ReleaseStringUTFChars( dstfilepath, Stringp );
 
	ret= (jobjectArray)env->NewObjectArray(mFiles.size(),env->FindClass("java/lang/String"),0);

	for(int i=0;i<mFiles.size();i++) 
		env->SetObjectArrayElement(ret,i,env->NewStringUTF( mFiles[i].c_str() ));

	mFiles.clear();

    return(ret);

}


  int indexer = 0;
JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_CreateTextureFromPath(JNIEnv * env, jobject obj, jstring filepath  ) { 



	if( mImagePos.size() <= 0 ) return;

	const char *Stringp = env->GetStringUTFChars(  filepath, NULL );
	std::string videofilepath =  std::string( Stringp );
	env->ReleaseStringUTFChars( filepath, Stringp );
 
 
	int posOutput = 0 + (rand() % (int)( (mImagePos.size()-1) - 0 + 1));
 
	float randX = NxUtils::GetRandom(-5.0f, 5.0f) ;
	float randY = NxUtils::GetRandom(-5.0f, 5.0f) ;

    LOGD( "----- CREATING TEXTURE : "   );
    float Z = mImagePos[posOutput].x + randX;//  3767;//NxUtils::GetRandom(3000, 4500) ;
    float X =  mImagePos[posOutput].y + randY;//700;//NxUtils::GetRandom(-1000, 1000) ;
	
	mImagePos.erase(mImagePos.begin() + posOutput );
	
 
    NxNode * triggerNode = Scene3DPlayer->CreateNxNode( "trigger" + NxVideoUtils::ToString( indexer ) );
	triggerNode->SetPosition( Nx::Vector3( X , 100 , Z)); 
	NxEntity * tester = triggerNode->CreateNxBox( "triggerEntoty" + NxVideoUtils::ToString( indexer), Nx::Vector3(110,85,2 ), Nx::Vector3(1,1,1) );
    Nx::Quaternion y180 = Nx::Quaternion( Nx::Degree(90.0f), Nx::Vector3::UNIT_Z);// 180.0f
    tester->SetOrientation(  y180  );
    NxTextureImage * tex =  NxTextureManager::getSingleton().CreateTextureImage( "texturePhoto" + NxVideoUtils::ToString( indexer), videofilepath );
	MaterialNx * MatLeft =  NxMaterialManager::getSingleton().CreateMaterial("photoMaterial" + NxVideoUtils::ToString( indexer) );
	NxTechnique * MatLeftTechnique = MatLeft->CreateTechnique("");
	NxPass * MatLeftPass =  MatLeftTechnique->CreatePass("");
	MatLeftPass->SetLightingEnabled(false);
	NxTextureUnit * MatLeftUnit = MatLeftPass->CreateTextureUnit("");
	MatLeftUnit->SetTextureName( "texturePhoto" + NxVideoUtils::ToString( indexer) );
	
	
	
	Nx::Matrix4 xform = Nx::Matrix4::IDENTITY;
	Nx::Quaternion qx, qy, qz, qfinal;
	qx.FromAngleAxis(Nx::Degree( 0 ), Nx::Vector3::UNIT_X);
	qy.FromAngleAxis(Nx::Degree( 0 ), Nx::Vector3::UNIT_Y);
	qz.FromAngleAxis(Nx::Degree( 0 ), Nx::Vector3::UNIT_Z);
	qfinal = qx * qy * qz;

	Nx::Vector3 translate;
	translate.x = 0.25;
	translate.y = 0;
	translate.z = 0;

	Nx::Matrix3 rot3x3, scale3x3;
	qfinal.ToRotationMatrix(rot3x3);
	scale3x3 = Nx::Matrix3::ZERO;
	scale3x3[0][0] = 0.75f;
	scale3x3[1][1] = 1.0f;
	scale3x3[2][2] = 1.0f;

	xform = rot3x3 * scale3x3;
	xform.setTrans(translate);

	MatLeftUnit->SetTextureTransform( xform );	
	
	
	
	///MatLeftUnit->SetTextureScale( 1.0f,  1.0f );
	
	
    tester->SetMaterialName( MatLeft->GetName() ); 
    indexer++;

}



JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_OpenTheoraVideo(JNIEnv * env, jobject obj, jstring filepath  ) {

	const char *Stringp = env->GetStringUTFChars( filepath, NULL );
	std::string videofilepath =  std::string( Stringp );
	env->ReleaseStringUTFChars( filepath, Stringp );


	if( PlaneNode != NULL ) {

		NxTextureManager::getSingleton().Remove("TheoraTexture"); 

		NxVideoPixelFormatInfo info; 
		info.mDstVideoFormat = NxVideoPixelFormaty420; //theora
		bool useopengl = true;
		NxTextureVideo *  mVideo = NxTextureManager::getSingleton().CreateTextureVideo( "TheoraTexture" , videofilepath, info , useopengl );
		mVideo->Play();

		 NxPlaneVideo->SetMaterialName( MatVideo->GetName() );


		MatVideoUnit0->SetTextureName( mVideo->GetTextureName(0) );
		MatVideoUnit1->SetTextureName( mVideo->GetTextureName(1) );
		MatVideoUnit2->SetTextureName( mVideo->GetTextureName(2) );
		 
		MatVideoPass->SetVertexAutoParameterValue( "worldViewProj", ACT_WORLDVIEWPROJ_MATRIX ); 
		MatVideoPass->SetFragmentParameterValue( "tex0", 0 );
		MatVideoPass->SetFragmentParameterValue( "tex1", 1 );
		MatVideoPass->SetFragmentParameterValue( "tex2", 2 ); 
        
         NxScreenModel->SetMaterialName(  MatVideo->GetName() ); // MatVideo
        
        
        



	}else{

		PlaneNode = Scene3DPlayer->CreateNxNode( "PlaneVideo" );
		NxPlaneVideo = PlaneNode->CreateNxPlane( "PlaneVideoGeometry", Nx::Vector2(widthVideo,widthVideo/2), Nx::Vector2(3,3) );

		

		MatVideo =  NxMaterialManager::getSingleton().CreateMaterial("videomaterial");
		NxTechnique * MatVideoTechnique = MatVideo->CreateTechnique("");
		MatVideoPass =  MatVideoTechnique->CreatePass("");
		//MatVideoPass->SetSceneBlending(  NXBLEND_TRANSPARENT_ALPHA );
		MatVideoPass->SetCullingMode(NXCULL_NONE);
		//MatVideoPass->SetDepthCheckEnabled(true);
		//MatVideoPass->SetDepthWriteEnabled(true);
        
        
        /*
        
         MatVideoUnit0 = MatVideoPass->CreateTextureUnit("");
		MatVideoUnit0->SetTextureAddressingMode(TEXTURE_BORDER);
		MatVideoUnit0->SetTextureFiltering( TFO_NONE);
        
        
		MatVideoUnit1 = MatVideoPass->CreateTextureUnit("");
		MatVideoUnit1->SetTextureAddressingMode(TEXTURE_BORDER);
		MatVideoUnit1->SetTextureFiltering( TFO_NONE);

		MatVideoUnit2 = MatVideoPass->CreateTextureUnit("");
		MatVideoUnit2->SetTextureAddressingMode(TEXTURE_BORDER);
		MatVideoUnit2->SetTextureFiltering( TFO_NONE);
        
        
        
        NxPlaneVideo->SetMaterialName( MatVideo->GetName() );
        
        
	NxVertexShader * vert = NxMaterialManager::getSingleton().CreateVertexProgram( "yuvtorgbvert", "glsles" );
		NxPixelShader * frag = NxMaterialManager::getSingleton().CreateFragmentProgram( "yuvtorgbfrag", "glsles" ); 

		vert->SetSource( vShaderStr );
		vert->Load();

		frag->SetSource( fShaderStr );
		frag->Load();

		MatVideoPass->SetVertexProgram(vert->GetName());
		MatVideoPass->SetFragmentProgram(frag->GetName() ); 

		NxVideoPixelFormatInfo info; 
		info.mDstVideoFormat = NxVideoPixelFormaty420; //theora
		bool useopengl = true;
		NxTextureVideo *  mVideo = NxTextureManager::getSingleton().CreateTextureVideo( "TheoraTexture" , videofilepath, info , useopengl );
		mVideo->Play();

		MatVideoUnit0->SetTextureName( mVideo->GetTextureName(0) );
		MatVideoUnit1->SetTextureName( mVideo->GetTextureName(1) );
		MatVideoUnit2->SetTextureName( mVideo->GetTextureName(2) );

		MatVideoPass->SetVertexAutoParameterValue( "worldViewProj", ACT_WORLDVIEWPROJ_MATRIX ); 
		MatVideoPass->SetFragmentParameterValue( "tex0", 0 );      */  


        
		 MatVideoUnit0 = MatVideoPass->CreateTextureUnit("");
		MatVideoUnit0->SetTextureAddressingMode(TEXTURE_BORDER);
		MatVideoUnit0->SetTextureFiltering( TFO_NONE);

		MatVideoUnit1 = MatVideoPass->CreateTextureUnit("");
		MatVideoUnit1->SetTextureAddressingMode(TEXTURE_BORDER);
		MatVideoUnit1->SetTextureFiltering( TFO_NONE);

		MatVideoUnit2 = MatVideoPass->CreateTextureUnit("");
		MatVideoUnit2->SetTextureAddressingMode(TEXTURE_BORDER);
		MatVideoUnit2->SetTextureFiltering( TFO_NONE);

		NxPlaneVideo->SetMaterialName( MatVideo->GetName() );

		//create 2 shaders
		NxVertexShader * vert = NxMaterialManager::getSingleton().CreateVertexProgram( "yuvtorgbvert", "glsles" );
		NxPixelShader * frag = NxMaterialManager::getSingleton().CreateFragmentProgram( "yuvtorgbfrag", "glsles" ); 

		vert->SetSource( vShaderStr );
		vert->Load();

		frag->SetSource( fShaderStrSimple );
		frag->Load();

		MatVideoPass->SetVertexProgram(vert->GetName());
		MatVideoPass->SetFragmentProgram(frag->GetName() ); 

		NxVideoPixelFormatInfo info; 
		info.mDstVideoFormat = NxVideoPixelFormaty420; //theora
		bool useopengl = true;
		NxTextureVideo *  mVideo = NxTextureManager::getSingleton().CreateTextureVideo( "TheoraTexture" , videofilepath, info , useopengl );
		mVideo->Play();

		MatVideoUnit0->SetTextureName( mVideo->GetTextureName(0) );
		MatVideoUnit1->SetTextureName( mVideo->GetTextureName(1) );
		MatVideoUnit2->SetTextureName( mVideo->GetTextureName(2) );

		MatVideoPass->SetVertexAutoParameterValue( "worldViewProj", ACT_WORLDVIEWPROJ_MATRIX ); 
		MatVideoPass->SetFragmentParameterValue( "tex0", 0 );
		MatVideoPass->SetFragmentParameterValue( "tex1", 1 );
		MatVideoPass->SetFragmentParameterValue( "tex2", 2 );
        
        
        // NxScreenModel->SetMaterialName(  MatVideo->GetName() ); // MatVideo
        
        
       


	}
	








	NxPlaneVideo->SetVisible(false);


	 

}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_CloseTheoraVideo(JNIEnv * env, jobject obj  ) {

	NxTextureManager::getSingleton().Remove("TheoraTexture"); 

}



 

JNIEXPORT jint JNICALL Java_com_hotstuff_main_OgreActivityJNI_CreateBackGroundTexture( JNIEnv * env, jobject obj ) {


	QCAR::VideoBackgroundTextureInfo videodata = QCAR::Renderer::getInstance().getVideoBackgroundTextureInfo();  
	int TextureWidth  = videodata.mTextureSize.data[0];
	int TextureHeight = videodata.mTextureSize.data[1];
	int VideoWidth  = videodata.mImageSize.data[0];
	int VideoHeight = videodata.mImageSize.data[1];
	QCAR::PIXEL_FORMAT pixelformat =  videodata.mPixelFormat;

	// Setup background UVs to match texture info:
	float scaleFactorX = (float)VideoWidth/(float)TextureWidth;//  640.0f / 1024.0f;
	float scaleFactorY = (float)VideoHeight/(float)TextureHeight;//480.0f / 512.0f;

	float c = 0.0f;
	float r = 0.0f;
	// 4 vertices.
	int numCols = 2;
	int numRows = 2;

	Nx::Vector2 topleft = Nx::Vector2(((float)c) / ((float)(numCols-1)) * scaleFactorX, ((float)r) / ((float)(numRows-1)) * scaleFactorY);
	Nx::Vector2 bottomLeft =   Nx::Vector2(((float)c) / ((float)(numCols - 1)) * scaleFactorX, ((float)(r + 1)) / ((float)(numRows - 1)) * scaleFactorY);
	Nx::Vector2 topRight =  Nx::Vector2(((float)(c + 1)) / ((float)(numCols - 1)) * scaleFactorX, ((float)r) / ((float)(numRows - 1)) * scaleFactorY);
	Nx::Vector2 bottomRight = Nx::Vector2(((float)(c + 1)) / ((float)(numCols - 1)) * scaleFactorX,((float)(r + 1)) / ((float)(numRows - 1)) * scaleFactorY);
	mBackgroundRectangle->SetUVs( topleft, bottomLeft, topRight, bottomRight );

	// Create the background texture
	LOGD("----> Creating material with parameters width: %d, height: %d, pixelformat: %d ", TextureWidth, TextureHeight, pixelformat   );
	NxPixelFormat format = NXPF_R5G6B5; 
	// NxTexture * tex = NxTextureManager::getSingleton().CreateTexture( "backtexture1", TextureWidth, TextureHeight, format );
	NxTexture * tex = NxTextureManager::getSingleton().CreateTexture( "backtexture1", "NxMaterialVideo", TextureWidth, TextureHeight, 0, NXTEX_TYPE_2D, format, NXTU_DYNAMIC_WRITE_ONLY_DISCARDABLE  );

	//const std::string & TexName, const std::string & textureGroupName, int Width = 256, int Height = 256, int numMips = 0, NxTextureType type = NXTEX_TYPE_2D, NxPixelFormat format = NXPF_R8G8B8, NxTextureUsage usage = NXTU_DEFAULT

//ENABLE HERE TO MAKE WORK BACKGROUND
	//mBackGroundUnit->SetTextureName( tex->GetTextureName() );
	
	//mBackGroundUnit->SetTextureName( "blacker" );

	int textureID = tex->GetTextureID(0);
	if(!QCAR::Renderer::getInstance().setVideoBackgroundTextureID( textureID )) {
		LOGD("---> ERROR : Renderer.getInstance().setVideoBackgroundTextureID : %d", textureID );
	}

	return textureID;


}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_ViewportSetCurrent(JNIEnv * env, jobject obj )
{
	NxEngine::getSingleton().GetNxWindow()->UpdateWindowMetrics();
}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_ViewportSetClearEveryFrame(JNIEnv * env, jobject obj, jboolean val )
{
	NxEngine::getSingleton().GetNxViewport()->SetClearEveryFrame( val ); 
}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_ViewportClear(JNIEnv * env, jobject obj)
{
	NxEngine::getSingleton().GetNxViewport()->Clear( NXFBT_COLOUR | NXFBT_DEPTH, NxColourValue(0,0,0,1) );
}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_ViewportSize(JNIEnv * env, jobject obj, jfloat x, jfloat y, jfloat width, jfloat height )
{
	NxEngine::getSingleton().GetNxViewport()->SetDimensions( x, y , width, height );
}

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_ViewportUpdate(JNIEnv * env, jobject obj)
{
	NxEngine::getSingleton().GetNxViewport()->Update();
}



float inc = 0;

JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_renderOneFrame(JNIEnv * env, jobject obj)
{
	if( isReady ) {
		
		if(gVM->AttachCurrentThread(&env, NULL) < 0) return;


		/*

		for( int i = 0 ; i < numBoxes; i++ ) {

			Nx::Quaternion quat;
			float dir = mBoxesDirection[i];
			quat.FromAngleAxis(Nx::Degree(mBoxesRotValues[i]),Nx::Vector3(dir,dir,dir));
			mBoxes[i]->Rotate( quat , NxWorld );
		}*/
 
		
	

		
			
		NxGraph->Update();
		
		
		


	}
}

Nx::Vector2 GetScreenDimensions() {

	Nx::Vector2 size;
	QCAR::VideoBackgroundConfig config = QCAR::Renderer::getInstance().getVideoBackgroundConfig();
	size.x = config.mSize.data[0];
	size.y = config.mSize.data[1];
}
 

bool misFirstScene = true;
 
JNIEXPORT void JNICALL Java_com_hotstuff_main_OgreActivityJNI_SetModelPose(JNIEnv * env, jobject obj, jfloatArray mat  ) {
 
	jfloat* fltmat = env->GetFloatArrayElements( mat ,0);
	
	QCAR::Matrix44F modelViewMatrix;
	for( int i = 0 ; i < 16; i++ ) {
		modelViewMatrix.data[i] =  fltmat[i];
	}	
	Nx::Matrix4 modelViewMatrixOgre;
	VuforiaMatrix2Nx( modelViewMatrix, modelViewMatrixOgre );
    
    /* extract camera position
    QCAR::Matrix44F inverseMV = SampleMath::Matrix44FInverse(modelViewMatrix);
    QCAR::Matrix44F invTranspMV = SampleMath::Matrix44FTranspose(inverseMV);
     // Extract the camera position from the last column of the matrix computed before the following occurs:
    float cam_x = invTranspMV.data[12];
    float cam_y = invTranspMV.data[13];
    float cam_z = invTranspMV.data[14];
    */
    
	/*
	
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetPosition(Nx::Vector3(0, 300, 0));
	//Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->LookAt(Nx::Vector3(0,300,-2500.0f));
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetNearPlane(10.0f);
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetFarPlane(5000.0f);//500 ok
	Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetFieldOfView( 90.0f  );//GetFovDegrees() );*/
	
	Nx::Matrix4 transform = Nx::Matrix4::IDENTITY;
	transform.setTrans( Nx::Vector3( 0, 0, widthBox/2 )  ); // << add offset Z here
	transform.setScale( Nx::Vector3( 1, 1, 1 ) );
	Nx::Matrix4 res = modelViewMatrixOgre * transform;
	Nx::Vector3 position = Nx::Vector3(res[0][3], -res[1][3]  , -res[2][3]     );

	Nx::Vector3 xr = Nx::Vector3((fltmat[0]), -(fltmat[1]), -(fltmat[2]));
	Nx::Vector3 yr = Nx::Vector3(( fltmat[4]), -(fltmat[5]), -(fltmat[6]));
	Nx::Vector3 zr = Nx::Vector3(( fltmat[8]), -(fltmat[9]), -(fltmat[10]));
	Nx::Vector3 x = xr.normalisedCopy();
	Nx::Vector3 y = yr.normalisedCopy();
	Nx::Vector3 z = zr.normalisedCopy();
	Nx::Quaternion orientation = Nx::Quaternion(x, y, z);
	
	
	if( NxModel ) { 
        
        //Nx::Quaternion y180 = Nx::Quaternion( Nx::Degree(0.0f), Nx::Vector3::UNIT_Y);// 180.0f
        
		
        ScreenNode->SetOrientation( Nx::Quaternion::IDENTITY );
        ScreenNode->SetPosition( Nx::Vector3(0,0,0));
		//NxScreenModel->SetOrientation(  y180  ); // right orientation  
        
        
		NxModel->SetOrientation( Nx::Quaternion::IDENTITY );
		NxModel->SetPosition( Nx::Vector3(0,0,0));
		//ModelNode->SetOrientation(  y180  ); // right orientation
        
        
        float ZReduc = 700.0f;//500.0f;
 
        position.z = position.z + ZReduc ;
 
        if( !canShowSecondRoom && misFirstScene && position.z < mThresholdDistance ) {
			
			position.y = 180.0f; // height
			position.x = 0.0f;

			if( mScreenTextureUnit )  { 
		  
				MatLeftPass1->SetFragmentParameterValue( "alpha", (float) (position.z / mThresholdDistance) +0.5f  );
				//LOGD( "%f" , position.z / mThresholdDistance );

			}

            Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetPosition(  position  );
 
        }else {  // second room
		
			if( misFirstScene == true ) fadePhoto = true;
			misFirstScene = false;
			
			
		
			if( canShowSecondRoom ) { 
				Scene3DPlayer->GetNxNode("CameraEditorNode")->GetNxController("CameraEditor")->SetPosition(  Nx::Vector3(  -15, 300, 3021 )  );
				
				if( !launchEndingTimer ) launchEndingTimer = true;
			}
            
            
            
            
        
        
            
        }
        
   
        
	}
	

	
	
	env->ReleaseFloatArrayElements(mat, fltmat, 0);
	

	return;

 
}


 
 
 
 
