#ifndef GUIANIMATION_H_INCLUDED
#define GUIANIMATION_H_INCLUDED

#include "GlobalTypes.h"
#include <string>
#include <sstream>


class GUIAnimationManager;
class GUIAnimation;


enum eGUIAnimProperty 
{
	GAP_POSITION = 0, 
	GAP_SCALE, 
	GAP_ANGLES, 
	GAP_ALPHA, 
	GAP_COLOR
};

enum eGUIAnimOrder 
{
	GAO_MODAL = 0, 
	GAO_FRONT = 250, 
	GAO_MIDDLE = 500, 
	GAO_BACK = 750
};

enum eUIxAnchor { UXA_LEFT, UXA_RIGHT, UXA_CENTER };
enum eUIyAnchor { UYA_TOP, UYA_BOTTOM, UYA_CENTER };
enum eUIPrecision { UIP_PERCENTAGE, UIP_PIXEL };

// GUI Input Callbacks
typedef void (*pfGUITouchAnimationCallback)(GUIAnimation* pSender);
// Easing Task Callback
typedef void (*pfGUIEasingAnimationCallback)(GUIAnimation* pSender,eGUIAnimProperty eProperty);
// Easing function type
typedef float (*pfEase)(const float fT);


// Helper classes

struct UIRelative
{
	static float XPercentFrom( const eUIxAnchor eAnchor, const float fWidth, const float fPercentOffset );
	static float YPercentFrom( const eUIyAnchor eAnchor, const float fHeight, const float fPercentOffset );
	static float XPercentTo( const eUIxAnchor eAnchor, const float fWidth, const float fOffset );
	static float YPercentTo( const eUIyAnchor eAnchor, const float fHeight, const float fOffset );

	static float XAnchorAdjustment( const eUIxAnchor eAnchor, const float fWidth, const eUIxAnchor eOriginAnchor );
	static float YAnchorAdjustment( const eUIyAnchor eAnchor, const float fHeight, const eUIyAnchor eOriginAnchor );
};


namespace Easing
{
	struct Linear
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};

	struct Quartic
	{
		static float EaseIn( const float fT ); 
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};

	struct Quintic
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};

	struct Sinusoidal
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};

	struct Exponential
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};

	struct Circular
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};

	struct Back
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );

	private:
		static float s_fS;
		static float s_fS2;
	};

	struct Bounce
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );

	private:
		static float s_fB;
		static float s_fC;
		static float s_fD;
	};

	struct Elastic
	{
		static float EaseIn( const float fT );
		static float EaseOut( const float fT );
		static float EaseInOut( const float fT );
	};
};

class EasingAnimation 
{
public:
	EasingAnimation( GUIAnimation* pGUIObject, const float fStartTimeOut, const float fDuration, eGUIAnimProperty eProperty, const hkvVec2 v2Start, const hkvVec2 v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfCallback = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation( GUIAnimation* pGUIObject, const float fStartTimeOut, const float fDuration, eGUIAnimProperty eProperty, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfCallback = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation( GUIAnimation* pGUIObject, const float fStartTimeOut, const float fDuration, eGUIAnimProperty eProperty, const VColorRef tStart, const VColorRef tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfCallback = 0, const bool bAffectedByTimeScale = true );
	~EasingAnimation();


	void Update( const float fDeltaTime );
	void OnFinished();

	void Play() { m_bRunning = true; }
	void Pause() { m_bRunning = false; }
	void Stop() { m_bRunning = false; m_bFinished = true; }
	bool IsFinished() const { return m_bFinished; }
	eGUIAnimProperty GetAnimProperty() const { return m_eAnimProperty; }
	void SetAutoreverse( const bool bAutoreverse ) { m_bAutoreverse = bAutoreverse; }

private:
	pfGUIEasingAnimationCallback m_pfCallback;
	pfEase m_pfEase;

	GUIAnimation* m_pGUIObject;
	eGUIAnimProperty m_eAnimProperty;
	float m_fStartTime;
	float m_fDuration;
	bool m_bRunning;
	bool m_bFinished;
	bool m_bAffectedByTimeScale;
	bool m_bAutoreverse;

	hkvVec2 m_v2Start, m_v2Target;
	float m_fStart, m_fTarget;
	VColorRef m_tStart, m_tTarget;
};


class GUIAnimation : public VUserDataObj
{
	friend class GUIAnimationManager;

public:
	struct FrameRect
	{
		FrameRect()
			: m_fX(0.f), m_fY(0.f), m_fW(0.f), m_fH(0.f) {}

		FrameRect( float x, float y, float w, float h ) 
			: m_fX(x), m_fY(y), m_fW(w), m_fH(h) {}

		void Init() { m_fX = 0.f; m_fY = 0.f; m_fW = 0.f; m_fH = 0.f; }

		FrameRect& operator=( const FrameRect& sOther )
		{
			m_fX = sOther.m_fX;
			m_fY = sOther.m_fY;
			m_fW = sOther.m_fW;
			m_fH = sOther.m_fH;
			return *this;
		}

		bool operator==( const FrameRect& sOther ) const
		{
			return ( m_fX == sOther.m_fX 
				&& m_fY == sOther.m_fY 
				&& m_fW == sOther.m_fW 
				&& m_fH == sOther.m_fH );
		}

		bool operator!=( const FrameRect& sOther ) const
		{
			return ( m_fX != sOther.m_fX 
				&& m_fY != sOther.m_fY 
				&& m_fW != sOther.m_fW 
				&& m_fH != sOther.m_fH );
		}


		void Set( const float fX, const float fY, const float fW, const float fH ) { m_fX = fX; m_fY = fY; m_fW = fW; m_fH = fH; }

		bool IsInside( const float fX, const float fY ) const
		{
			return ( fX >= m_fX 
				&& fX <= (m_fX + m_fW) 
				&& fY >= m_fY 
				&& fY <= (m_fY + m_fH) );
		}

		bool IsValid() const
		{
			return !IsZero();
		}

		bool IsZero() const
		{
			return ( m_fX == 0.f 
				&& m_fY == 0.f 
				&& m_fW == 0.f 
				&& m_fH == 0.f );
		}

		float m_fX, m_fY, m_fW, m_fH;
	};


	enum eGUIAnimType
	{
		GAT_NONE = -1,
		GAT_LOOP = 0, 
		GAT_ONCE, 
		GAT_PING_PONG
	};


	struct UIAnchorInfo
	{
		void Init()
		{
			m_pParent = 0;
			m_eParentUIxAnchor = UXA_LEFT;
			m_eParentUIyAnchor = UYA_TOP;
			m_eUIxAnchor = UXA_LEFT;
			m_eUIyAnchor = UYA_TOP;
			m_eOriginUIxAnchor = UXA_LEFT;
			m_eOriginUIyAnchor = UYA_TOP;
			m_eUIPrecision = UIP_PERCENTAGE;
			m_fOffsetX = 0.f;
			m_fOffsetY = 0.f;
		}


		GUIAnimation* m_pParent;
		eUIxAnchor m_eParentUIxAnchor;
		eUIyAnchor m_eParentUIyAnchor;
		eUIxAnchor m_eUIxAnchor;
		eUIyAnchor m_eUIyAnchor;
		eUIxAnchor m_eOriginUIxAnchor;
		eUIyAnchor m_eOriginUIyAnchor;
		eUIPrecision m_eUIPrecision;
		float m_fOffsetX;
		float m_fOffsetY;
	};


private:
	GUIAnimation( const std::string& sFilename, const int iFirstFrame, const int iLastFrame, const int iNumFrames, const eGUIAnimID eID, const eGUIAnimType eType );


	void RefreshPosition();
	void PositionFromCenter( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromTopLeft( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromTopRight( const float fPercentFromTop, const float fPercentFromRight, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromBottomLeft( const float fPercentFromBottom, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromBottomRight( const float fPercentFromBottom, const float fPercentFromRight, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromLeft( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromTop( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromBottom( const float fPercentFromBottom, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	void PositionFromRight( const float fPercentFromTop, const float fPercentFromRight, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor );
	hkvVec2 ParentAnchorPosition() const;
	float ParentWidth() const;
	float ParentHeight() const;

	EasingAnimation* To( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true  );
	EasingAnimation* To( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* To( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* From( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* From( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* From( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Start, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* FromTo( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* FromTo( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tStart, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* FromTo( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Start, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );

	EasingAnimation* Animate( const bool bAnimateTo, const float fStartTimeOut,  const float fDuration, const eGUIAnimProperty eProperty, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* Animate( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* Animate( const bool bAnimateTo, const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* Animate( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& fStart, const VColorRef& fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* Animate( const bool bAnimateTo, const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );
	EasingAnimation* Animate(const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& fStart, const hkvVec2& fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale = true );

	void UpdateInput();
	void UpdateAnimation( const float fDeltaTime );

	void OverrideAnimTypeIfExists( const eGUIAnimProperty eProperty ) { for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) if ( m_aEasingAnims[i]->GetAnimProperty() == eProperty && !m_aEasingAnims[i]->IsFinished() ) m_aEasingAnims[i]->Stop(); }

public:
	virtual ~GUIAnimation();

	static GUIAnimation* Create( const std::string& sSrcTextureFilepath, const std::string& sFilename, const int iFirstFrame, const int iLastFrame, const int iNumFrames, const eGUIAnimID eID, const eGUIAnimType eType = GAT_ONCE );
	static void SetMapTouchArea( const float fX, const float fY, const float fWidth, const float fHeight, eGUIAnimID eID );


	void SetFPS( int iFPS ) { m_iAnimFPS = iFPS; }
	int GetFPS() const { return m_iAnimFPS; }
	void SetFrameRange( int iFirstFrame, int iLastFrame ) { m_iFirstFrame = iFirstFrame; m_iLastFrame = iLastFrame; }
	int GetFirstFrame() const { return m_iFirstFrame; }
	int GetLastFrame() { return m_iLastFrame; }
	void SetCurrentFrame( int iFrame ) { m_iCurrentFrame = iFrame; }
	int GetCurrentFrame() const { return m_iCurrentFrame; }
	void SetAnimType( const eGUIAnimType eType ) { m_eType = eType; }
	eGUIAnimType GetAnimType() const { return m_eType; }
	eGUIAnimID GetAnimID() const { return m_eID; }
	int GetNumFrames() const { return m_iNumFrames; }
	bool IsActiveFrameAnim() const { return m_bActiveFrameAnim; }
	bool IsActiveEaseAnim() const { return m_bActiveEaseAnim; }
	bool IsActiveAnim() const { return m_bActiveFrameAnim || m_bActiveEaseAnim; }
	bool IsRunning() const { return m_bRunning; }

	void Play();
	void Stop();
	void PlayBackward();
	void Update( float fDeltaTime );
	void PlayEasing( const eGUIAnimProperty eProperty );
	void PlayAllEasings();
	void PauseEasing( const eGUIAnimProperty eProperty );
	void PauseAllEasings();
	void StopEasing( const eGUIAnimProperty eProperty );
	void StopAllEasings();
	void RemoveEasingsFinished();

	void SetRenderFrame( const unsigned int iFramePos );
	void AddFrameRect( const FrameRect& tFrame ) { m_aFrameRects[ m_aFrameRects.GetFreePos() ] = tFrame; }
	void AddFrameRect( const FrameRect& tFrame, int iPos ) { m_aFrameRects[ iPos ] = tFrame; }
	void AddFrameRects( const FrameRect atFrame[], int iSize ) { for ( int i = 0; i < iSize; i++ ) AddFrameRect( atFrame[i] ); }
	const FrameRect& GetFrameRect( const int iPos ) const { return m_aFrameRects[iPos]; }
	int GetFrameRectPos( const FrameRect& tFrame ) const { for ( unsigned int i = 0; i < m_aFrameRects.GetValidSize(); i++ ) if ( m_aFrameRects[i] == tFrame ) return i; return -1; }

	const std::string& GetFilename() const { return m_sFilename; }
	void SetTextureOnce( VisScreenMask_cl* pTexture ) { if ( m_spTexture ) return; m_spTexture = pTexture; }
	VisScreenMask_cl* GetTexture() { return m_spTexture; }
	const VisScreenMask_cl* GetTexture() const { return m_spTexture; }
	void SetOrder( const int iPos );
	void SetZVal( const float fZ ) { if ( m_spTexture ) m_spTexture->SetZVal( fZ ); }
	void SetVisible( const bool bIsVisible );
	void SetOnTouchUpSound( const eSounds eSound ) { m_eOnTouchUpSound = eSound; }
	void SetOnEasingCompleteSound( const eSounds eSound ) { m_eOnEasingCompleteSound = eSound; }
	void SetOnEasingStartSound( const eSounds eSound ) { m_eOnEasingStartSound = eSound; }
	void SetTouchable( bool bTouchable ) { m_bTouchable = bTouchable; }
	bool IsTouchable() const { return m_bTouchable; }
	BOOL IsVisible() const { return m_spTexture->IsVisible(); }
	bool IsTouched() const { return m_bTouched; }

	void DemandRefreshTouchArea() { RefreshTouchArea(); }
	bool IsTouchAreaDirty() const { return m_bTouchAreaIsDirty; }
	UIAnchorInfo& GetAnchorInfo() { return m_tAnchorInfo; }
	const UIAnchorInfo& GetAnchorInfo() const { return m_tAnchorInfo; }
	void SetParent( GUIAnimation* pAnimParent ) { m_tAnchorInfo.m_pParent = pAnimParent; m_bTouchAreaIsDirty = true; }
	void GetPosition( float& fX, float& fY ) const { fX = m_tTouchArea.m_fX; fY = m_tTouchArea.m_fY; } // FIXME: Return pos from actual current anchor
	void GetRelativePostition( float& fRelX, float& fRelY ) const;
	void GetRelativeSize( float& fRelW, float& fRelH ) const;
	void SetRotationAngle( const float fAngle ) { if ( m_spTexture ) m_spTexture->SetRotationAngle( fAngle ); }
	void InitializeSize();
	void SetSize( const float fWidth, const float fHeight );
	void SetRelativeSize( const float fNormWidth, const float fNormHeight );
	void SetScale( const float fScaleX, const float fScaleY ) { if ( m_spTexture ) { SetSize( m_fInitWidth * fScaleX, m_fInitHeight * fScaleY ); m_fScaleX = fScaleX; m_fScaleY = fScaleY; } }
	void GetScale( float& fScaleX, float& fScaleY ) const { float fWidth, fHeight; GetSize( fWidth, fHeight ); fScaleX = fWidth / m_fInitWidth; fScaleY = fHeight / m_fInitHeight; }
	hkvVec2 GetScale() const { float fScaleX, fScaleY; GetScale( fScaleX, fScaleY ); return hkvVec2( fScaleX, fScaleY ); }
	void GetSize( float& fWidth, float& fHeight ) const { fWidth = m_tTouchArea.m_fW; fHeight = m_tTouchArea.m_fH; }
	const hkvVec2 GetSize() const { return hkvVec2( m_tTouchArea.m_fW, m_tTouchArea.m_fH ); }
	void SetColor( const VColorRef& tColor ) { if ( m_spTexture ) m_spTexture->SetColor( tColor ); }
	
	void RefreshTouchArea();
	void PositionFromCenter( const float fPercentFromTop, const float fPercentFromLeft );
	void PositionFromTopLeft( const float fPercentFromTop, const float fPercentFromLeft );
	void PositionFromTopRight( const float fPercentFromTop, const float fPercentFromRight );
	void PositionFromBottomLeft( const float fPercentFromBottom, const float fPercentFromLeft );
	void PositionFromBottomRight( const float fPercentFromBottom, const float fPercentFromRight );
	void PositionFromTop( const float fPercentFromTop );
	void PositionFromTop( const float fPercentFromTop, const float fPercentFromLeft );
	void PositionFromBottom( const float fPercentFromBottom );
	void PositionFromBottom( const float fPercentFromBottom, const float fPercentFromLeft );
	void PositionFromLeft( const float fPercentFromLeft );
	void PositionFromLeft( const float fPercentFromTop, const float fPercentFromLeft );
	void PositionFromRight( const float fPercentFromRight );
	void PositionFromRight( const float fPercentFromTop, const float fPercentFromRight );

	void TryPlayOnEasingCompleteSound() const;
	void TryPlayOnEasingStartSound() const;

	// Input events callbacks
	void AddOnTouchUpCallback( pfGUITouchAnimationCallback _pfCallback );
	void AddOnTouchDownCallback( pfGUITouchAnimationCallback _pfCallback );

	// Input Events
	void OnTouchUp();
	void OnTouchDown();

	// Transform Animation functions
	EasingAnimation* AlphaTo( const float fStartTimeOut, const float fDuration, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* AlphaFrom( const float fStartTimeOut, const float fDuration, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* AlphaFromTo( const float fStartTimeOut, const float fDuration, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* ColorTo( const float fStartTimeOut, const float fDuration, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* ColorFrom( const float fStartTimeOut, const float fDuration, const VColorRef& tStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* ColorFromTo( const float fStartTimeOut, const float fDuration, const VColorRef& tStart, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* AnglesTo( const float fStartTimeOut, const float fDuration, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* AnglesFrom( const float fStartTimeOut, const float fDuration, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* AnglesFromTo( const float fStartTimeOut, const float fDuration, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* ScaleTo( const float fStartTimeOut, const float fDuration, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* ScaleFrom( const float fStartTimeOut, const float fDuration, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* ScaleFromTo( const float fStartTimeOut, const float fDuration, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* PositionTo( const float fStartTimeOut, const float fDuration, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* PositionFrom( const float fStartTimeOut, const float fDuration, const hkvVec2& v2Start, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );
	EasingAnimation* PositionFromTo( const float fStartTimeOut, const float fDuration, const hkvVec2& v2Start, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete = 0, const bool bAffectedByTimeScale = true );

private:
	int m_iAnimFPS;
	int m_iFirstFrame, m_iLastFrame;
	int m_iCurrentFrame;
	int m_iNumFrames;
	float m_fNextFrameTimer;
	eGUIAnimType m_eType;
	bool m_bActiveFrameAnim;
	bool m_bActiveEaseAnim;
	bool m_bRewinding;

	DynArray_cl<FrameRect> m_aFrameRects;
	std::string m_sFilename;
	VSmartPtr<VisScreenMask_cl> m_spTexture;
	eGUIAnimID m_eID;
	bool m_bRunning;
	DynArray_cl<EasingAnimation*> m_aEasingAnims;

	float m_fLastTouchXPos, m_fLastTouchYPos;
	bool m_bTouchable;
	bool m_bTouched;
	pfGUITouchAnimationCallback m_pfOnTouchUp;
	pfGUITouchAnimationCallback m_pfOnTouchDown;
	eSounds m_eOnTouchUpSound;
	eSounds m_eOnEasingCompleteSound;
	eSounds m_eOnEasingStartSound;

	UIAnchorInfo m_tAnchorInfo;
	FrameRect m_tTouchArea;
	bool m_bTouchAreaIsDirty;

	float m_fScaleX, m_fScaleY;
	float m_fInitWidth, m_fInitHeight;
};


#endif // GUIANIMATION_H_INCLUDED