#include "CutshumotoPluginPCH.h"
#include "GUIAnimation.h"
#include "GUIAnimationManager.h"
#include "SoundManager.h"

#include <Vision/Runtime/Base/Input/VInputTouch.hpp>

GUIAnimation::GUIAnimation( const std::string& sFilename, const int iFirstFrame, const int iLastFrame, const int iNumFrames, const eGUIAnimID eID, const eGUIAnimType eType ) : VUserDataObj()
{
	m_iAnimFPS = 24;
	m_iFirstFrame = iFirstFrame;
	m_iLastFrame = iLastFrame;
	m_iCurrentFrame = iFirstFrame;
	m_fNextFrameTimer = 0.f;
	m_eType = eType;
	m_bActiveFrameAnim = false;
	m_bActiveEaseAnim = false;
	m_bRewinding = false;
	m_aFrameRects.Init( FrameRect( 0.f, 0.f, 0.f, 0.f ) );
	m_eID = eID;
	m_sFilename = sFilename;
	m_spTexture = 0;
	m_iNumFrames = iNumFrames;
	m_bTouchable = true;
	m_pfOnTouchUp = 0;
	m_pfOnTouchDown = 0;
	m_bTouched = false;
	m_tTouchArea.Init();
	m_bTouchAreaIsDirty = true;
	m_tAnchorInfo.Init();
	m_fInitWidth = 0;
	m_fInitHeight = 0;
	m_aEasingAnims.Init(0);
	m_eOnTouchUpSound = eNoSound;
	m_eOnEasingCompleteSound = eNoSound;
	m_eOnEasingStartSound = eNoSound;
	m_fScaleX = 1.f;
	m_fScaleY = 1.f;
	m_fLastTouchXPos = 0.f;
	m_fLastTouchYPos = 0.f;
	
	if ( iNumFrames > 0 )
	{ // Loads frames from spritesheet
		int iExtensionDotPos = sFilename.rfind( "." );
		std::string sExtension = sFilename.substr( iExtensionDotPos + 1 );
		std::string sBaseFilename = sFilename.substr( 0, iExtensionDotPos );

		std::ostringstream ss;
		ss << sBaseFilename << "_" << 0 << "." << sExtension;
		FrameRect tMaxSize = GUIAnimationManager::Instance().GetFrame( ss.str() ); // Get the first frame
		if ( !tMaxSize.IsValid() ) 
		{ // Try to load without frame num extension
			FrameRect tStandalone = GUIAnimationManager::Instance().GetFrame( sFilename );
			AddFrameRect( tStandalone );
			m_eType = GAT_NONE;
			// Set init size
			m_fInitWidth = tStandalone.m_fW;
			m_fInitHeight = tStandalone.m_fH;
		}
		else
		{
			AddFrameRect( tMaxSize );
			for ( int i = 1; i < iNumFrames; i++ ) // Load all frames
			{
				// Clean ostringstream
				ss.clear();
				ss.seekp(0);
				// Build numbered filename
				ss << sBaseFilename << "_" << i << "." << sExtension;
				FrameRect tMaxToMatch = GUIAnimationManager::Instance().GetFrame( ss.str() );
				if ( tMaxToMatch.IsValid() ) AddFrameRect( tMaxToMatch );
				// Obtain max size
				if ( tMaxToMatch.m_fW > tMaxSize.m_fW ) tMaxSize.m_fW = tMaxToMatch.m_fW;
				if ( tMaxToMatch.m_fH > tMaxSize.m_fH ) tMaxSize.m_fH = tMaxToMatch.m_fH;
			}
			// Set init size
			m_fInitWidth = tMaxSize.m_fW;
			m_fInitHeight = tMaxSize.m_fH;
		}
	}
	else
	{ // Loads just one image
		m_eType = GAT_NONE;
	}
}

GUIAnimation::~GUIAnimation()
{
	// Free Easing Animations
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
	{
		if ( m_aEasingAnims[i] ) delete m_aEasingAnims[i];
		m_aEasingAnims.Remove(i);
	}
	m_aEasingAnims.Reset();
	// Free rect areas
	m_aFrameRects.Reset();
	// Free render tex
	m_spTexture = 0;
	// Unmap input trigger
	GUIAnimationManager::Instance().GetInputMap()->UnmapInput( m_eID );
}

GUIAnimation* GUIAnimation::Create( const std::string& sSrcTextureFilepath, const std::string& sFilename, const int iFirstFrame, const int iLastFrame, const int iNumFrames, const eGUIAnimID eID, const eGUIAnimType eType )
{
	GUIAnimation* pNewAnim = new GUIAnimation( sFilename, iFirstFrame, iLastFrame, iNumFrames, eID, eType );
	// Create render tex
	VisScreenMask_cl* pTex = new VisScreenMask_cl();
	BOOL bTexLoaded = false;
	if ( pNewAnim->GetNumFrames() == 0 ) bTexLoaded = pTex->LoadFromFile( (sSrcTextureFilepath + sFilename).c_str(), VTM_FLAG_NO_MIPMAPS ); // Load standalone tex
	else bTexLoaded = pTex->LoadFromFile( (sSrcTextureFilepath + TPTEXFILE_EXTENSION).c_str(), VTM_FLAG_NO_MIPMAPS ); // Load TexAtlas
	VASSERT( bTexLoaded );
	pTex->SetTransparency( VIS_TRANSP_ALPHA );
	// Add render tex
	pNewAnim->SetTextureOnce( pTex );
	// Set render frame
	pNewAnim->SetRenderFrame( iFirstFrame );
	
	// Initialize size
	pNewAnim->InitializeSize();

	return pNewAnim;
}

void GUIAnimation::InitializeSize()
{
	if ( !m_fInitWidth && !m_fInitHeight ) 
		m_spTexture->GetTargetSize( m_fInitWidth, m_fInitHeight ); // Init size not setted yet
	SetSize( m_fInitWidth, m_fInitHeight );
	RefreshTouchArea();
}

void GUIAnimation::Play()
{
	m_bActiveFrameAnim = true;
}

void GUIAnimation::Stop()
{
	m_bActiveFrameAnim = false;
	m_bRewinding = false;
	m_iCurrentFrame = m_iFirstFrame;
	m_bRunning = false;
}

void GUIAnimation::PlayBackward()
{
	m_bRewinding = true;
	m_bActiveFrameAnim = true;
	m_iCurrentFrame = m_iLastFrame;
}

void GUIAnimation::PlayEasing( const eGUIAnimProperty eProperty ) 
{
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
	{
		if ( !m_aEasingAnims[i]->IsFinished() && m_aEasingAnims[i]->GetAnimProperty() == eProperty ) 
		{
			m_aEasingAnims[i]->Play();
			break;
		}
	}
}

void GUIAnimation::PlayAllEasings() 
{
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
		if ( !m_aEasingAnims[i]->IsFinished() ) m_aEasingAnims[i]->Play();
}

void GUIAnimation::PauseEasing( const eGUIAnimProperty eProperty ) 
{
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
	{
		if ( !m_aEasingAnims[i]->IsFinished() && m_aEasingAnims[i]->GetAnimProperty() == eProperty ) 
		{
			m_aEasingAnims[i]->Pause();
			break;
		}
	}
}

void GUIAnimation::PauseAllEasings() 
{
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
		if ( !m_aEasingAnims[i]->IsFinished() ) m_aEasingAnims[i]->Pause();
}

void GUIAnimation::StopEasing( const eGUIAnimProperty eProperty ) 
{
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
	{
		if ( !m_aEasingAnims[i]->IsFinished() && m_aEasingAnims[i]->GetAnimProperty() == eProperty ) 
		{
			m_aEasingAnims[i]->Stop();
			break;
		}
	}
}

void GUIAnimation::StopAllEasings() 
{
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
		if ( !m_aEasingAnims[i]->IsFinished() ) m_aEasingAnims[i]->Stop();
}

void GUIAnimation::SetVisible( const bool bIsVisible ) 
{
	m_spTexture->SetVisible( bIsVisible );
	m_bTouchable = bIsVisible;
	// Clean pending stuff (like easing animations flaged as finished)
	RemoveEasingsFinished();
}

void GUIAnimation::RemoveEasingsFinished() 
{
	bool bRemoved = false;
	for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
	{
		if ( m_aEasingAnims[i] && m_aEasingAnims[i]->IsFinished() ) 
		{
			bRemoved = true;
			delete m_aEasingAnims[i];
			m_aEasingAnims.Remove(i);
		}
	}
	if ( bRemoved ) m_aEasingAnims.Pack();
	// If no ease animations left then set false activeEaseAnim
	if ( m_aEasingAnims.GetValidSize() == 0 ) m_bActiveEaseAnim = false;
}

void GUIAnimation::Update( float fDeltaTime )
{
	m_bRunning = true;

	// Update touch area if dirty
	if ( m_bTouchAreaIsDirty ) RefreshTouchArea();
	// Update animations
	if ( IsActiveAnim() ) UpdateAnimation( fDeltaTime );
}

void GUIAnimation::UpdateAnimation( const float fDeltaTime )
{
	if ( m_bActiveEaseAnim ) 
	{ // Perfom ease animation
		for ( unsigned int i = 0; i < m_aEasingAnims.GetValidSize(); i++ ) 
		{
			if ( m_aEasingAnims[i] ) 
			{
				if ( !m_aEasingAnims[i]->IsFinished() ) 
				{ // Update
					m_aEasingAnims[i]->Update( fDeltaTime );
				}
			}
		}
		RemoveEasingsFinished();
	}

	if ( m_bActiveFrameAnim ) 
	{ // Perfom frame animation
		if ( m_aFrameRects.GetValidSize() <= 1 || m_eType == GAT_NONE ) return;
			
		m_fNextFrameTimer += m_iAnimFPS * fDeltaTime;
		if ( m_fNextFrameTimer > 1 )
		{
			if ( m_bRewinding )
				m_iCurrentFrame--;
			else
				m_iCurrentFrame++;
			m_fNextFrameTimer = 0;

			if ( m_iCurrentFrame > m_iLastFrame )
				m_iCurrentFrame = m_iFirstFrame;
			if ( m_iCurrentFrame < m_iFirstFrame )
				m_iCurrentFrame = m_iLastFrame;

			switch ( m_eType )
			{
			case GAT_ONCE:

				if ( m_iCurrentFrame == m_iFirstFrame )
				{
					Stop(); // A full iteration completed
					return;
				}
				break;
			case GAT_PING_PONG:

				if ( m_iCurrentFrame == m_iLastFrame )
					m_bRewinding = !m_bRewinding; // Switch to backward anim
				else if ( m_iCurrentFrame == m_iFirstFrame )
				{
					Stop();
					return;
				}
				break;
			default:
				break;
			}

			// Set texture offset
			SetRenderFrame( m_iCurrentFrame );
		}
	}
}

void GUIAnimation::UpdateInput()
{
	#if defined(_VISION_MOBILE) // Mobile

	IVMultiTouchInput& inputDevice = static_cast<IVMultiTouchInput&>(VInputDeviceManager::GetInputDevice( INPUT_DEVICE_TOUCHSCREEN ));
	if ( inputDevice.GetNumberOfTouchPoints() > 0 ) 
	{
		const IVMultiTouchInput::VTouchPoint& touch = inputDevice.GetTouch(0);
		if ( m_tTouchArea.IsValid() && m_tTouchArea.IsInside( touch.fXAbsolute, touch.fYAbsolute ) ) 
		{
			if ( !m_bTouched ) 
				OnTouchDown();
			m_bTouched = true;
		}
		m_fLastTouchXPos = touch.fXAbsolute;
		m_fLastTouchYPos = touch.fYAbsolute;
	} 
	else 
	{
		if ( m_bTouched )
			OnTouchUp();
		m_bTouched = false;
	}
	/************************************************************************** MapTriggers dont working
	if ( GUIAnimationManager::Instance().GetInputMap()->GetTrigger( m_eID ) )
	{
		if ( !m_bTouched ) 
			OnTouchDown();
		m_bTouched = true;
	}
	else
	{
		if ( m_bTouched )
			OnTouchUp();
		m_bTouched = false;
	}
	************************************************************/
	#elif defined(SUPPORTS_MOUSE) // Win

	// Get mouse pos
	float fMouseX, fMouseY;
	Vision::Mouse.GetPosition( fMouseX, fMouseY );
	if ( Vision::Mouse.IsLeftButtonPressed() ) // Check action button pressed
	{
		if ( m_tTouchArea.IsValid() && m_tTouchArea.IsInside( fMouseX, fMouseY ) )
		{
			if ( !m_bTouched ) 
				OnTouchDown();
			m_bTouched = true;
		}
	}
	else
	{
		if ( m_bTouched )
			OnTouchUp();
		m_bTouched = false;
	}
	#endif
}

void GUIAnimation::AddOnTouchUpCallback( pfGUITouchAnimationCallback _pfCallback )
{
	m_pfOnTouchUp = _pfCallback;
}

void GUIAnimation::AddOnTouchDownCallback( pfGUITouchAnimationCallback _pfCallback )
{
	m_pfOnTouchDown = _pfCallback;
}

void GUIAnimation::OnTouchUp()
{
	GUIAnimationManager::Instance().m_pTouchHandler = 0; // Let free input event management

	if ( m_eOnTouchUpSound != eNoSound ) 
		SoundManager::Instance()->PlaySound( m_eOnTouchUpSound, Vision::Camera.GetMainCamera()->GetPosition(), false );

	if ( m_pfOnTouchUp )
		m_pfOnTouchUp( this );
}

void GUIAnimation::OnTouchDown()
{
	GUIAnimationManager::Instance().m_pTouchHandler = this;

	if ( m_pfOnTouchDown )
		m_pfOnTouchDown( this );
}

void GUIAnimation::SetOrder( const int iPos ) 
{
	if ( m_spTexture ) 
	{ 
		GUIAnimationManager::Instance().m_bAnimsArrayIsDirty = true;
		m_spTexture->SetOrder( iPos ); 
	}
}

void GUIAnimation::SetSize( const float fWidth, const float fHeight )
{
	if ( m_spTexture )
	{
		// Set size
		m_spTexture->SetTargetSize( fWidth, fHeight );
		// Update touch area
		RefreshTouchArea(); // FIXME: Use bTouchAreaIsDirty
	}
}

/*
	Returns normalized position relative to parent
	FIXME: Return pos from actual current anchor. Use touchArea
*/
void GUIAnimation::GetRelativePostition( float& fRelX, float& fRelY ) const 
{
	if ( m_spTexture ) 
	{ 
		float fX, fY;
		m_spTexture->GetPos( fX, fY );
		if ( m_tAnchorInfo.m_pParent )
		{ // Has parent
			float fParentW, fParentH;
			m_tAnchorInfo.m_pParent->GetSize( fParentW, fParentH );
			fRelX = fX / fParentW;
			fRelY = fY / fParentH;
		}
		else
		{ // Screen is the parent
			fRelX = fX / Vision::Video.GetXRes();
			fRelY = fY / Vision::Video.GetYRes();
		}
	}
	else
	{
		fRelX = -10000.f;
		fRelY = -10000.f;
	}
}

void GUIAnimation::GetRelativeSize( float& fRelW, float& fRelH ) const 
{
	if ( m_spTexture ) 
	{
		float iW, iH;
		m_spTexture->GetTargetSize( iW, iH );
		if ( m_tAnchorInfo.m_pParent ) 
		{
			float iParentW, iParentH;
			m_tAnchorInfo.m_pParent->GetTexture()->GetTargetSize( iParentW, iParentH );
			fRelW = iW / iParentW;
			fRelH = iH / iParentH;
		}
		else 
		{ // Screen as parent
			fRelW = iW / static_cast<float>(Vision::Video.GetXRes());
			fRelH = iH / static_cast<float>(Vision::Video.GetYRes());
		}
	}
	else 
	{ // Set not valid size
		fRelW = -1.f;
		fRelH = -1.f;
	}
}

void GUIAnimation::SetRelativeSize( const float fNormWidth, const float fNormHeight )
{
	if ( m_spTexture )
	{
		float fResultWidth, fResultHeight;
		if ( m_tAnchorInfo.m_pParent ) 
		{
			m_tAnchorInfo.m_pParent->GetSize( fResultWidth, fResultHeight );
		}
		else 
		{
			// Parent is the Screen
			fResultWidth = static_cast<float>(Vision::Video.GetXRes());
			fResultHeight = static_cast<float>(Vision::Video.GetYRes());
		}
		// Set size
		SetSize( fResultWidth * fNormWidth, fResultHeight * fNormHeight );
	}
}

void GUIAnimation::SetRenderFrame( const unsigned int iFramePos )
{
	if ( iFramePos >= m_aFrameRects.GetValidSize() ) return;  // Checks if position is within the limits

	if ( m_iCurrentFrame != iFramePos ) m_iCurrentFrame = iFramePos;
	const FrameRect& tFrame = m_aFrameRects[iFramePos];
	m_spTexture->SetTextureRange( tFrame.m_fX, tFrame.m_fY, tFrame.m_fX + tFrame.m_fW, tFrame.m_fY + tFrame.m_fH );
}

void GUIAnimation::PositionFromCenter( const float fPercentFromTop, const float fPercentFromLeft )
{
	PositionFromCenter( fPercentFromTop, fPercentFromLeft, UYA_CENTER, UXA_CENTER );
}

void GUIAnimation::PositionFromTopLeft( const float fPercentFromTop, const float fPercentFromLeft )
{
	PositionFromTopLeft( fPercentFromTop, fPercentFromLeft, UYA_TOP, UXA_LEFT );
}

void GUIAnimation::PositionFromTopRight( const float fPercentFromTop, const float fPercentFromRight )
{
	PositionFromTopRight( fPercentFromTop, fPercentFromRight, UYA_TOP, UXA_RIGHT );
}

void GUIAnimation::PositionFromBottomLeft( const float fPercentFromBottom, const float fPercentFromLeft )
{
	PositionFromBottomLeft( fPercentFromBottom, fPercentFromLeft, UYA_BOTTOM, UXA_LEFT );
}

void GUIAnimation::PositionFromBottomRight( const float fPercentFromBottom, const float fPercentFromRight )
{
	PositionFromBottomRight( fPercentFromBottom, fPercentFromRight, UYA_BOTTOM, UXA_RIGHT );
}

void GUIAnimation::PositionFromTop( const float fPercentFromTop )
{
	PositionFromTop( fPercentFromTop, 0.f, UYA_TOP, UXA_CENTER );
}

void GUIAnimation::PositionFromBottom( const float fPercentFromBottom )
{
	PositionFromBottom( fPercentFromBottom, 0.f, UYA_BOTTOM, UXA_CENTER );
}

void GUIAnimation::PositionFromLeft( const float fPercentFromLeft )
{
	PositionFromLeft( 0.f, fPercentFromLeft, UYA_CENTER, UXA_LEFT );
}

void GUIAnimation::PositionFromRight( const float fPercentFromRight )
{
	PositionFromRight( 0.f, fPercentFromRight, UYA_CENTER, UXA_RIGHT );
}

void GUIAnimation::PositionFromCenter( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_CENTER;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_CENTER;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromLeft;
	m_tAnchorInfo.m_fOffsetY = fPercentFromTop;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromTopLeft( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_LEFT;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_TOP;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromLeft;
	m_tAnchorInfo.m_fOffsetY = fPercentFromTop;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromTopRight( const float fPercentFromTop, const float fPercentFromRight, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_RIGHT;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_TOP;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromRight;
	m_tAnchorInfo.m_fOffsetY = fPercentFromTop;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromBottomLeft( const float fPercentFromBottom, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_LEFT;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_BOTTOM;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromLeft;
	m_tAnchorInfo.m_fOffsetY = fPercentFromBottom;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromBottomRight( const float fPercentFromBottom, const float fPercentFromRight, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_RIGHT;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_BOTTOM;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromRight;
	m_tAnchorInfo.m_fOffsetY = fPercentFromBottom;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromLeft( const float fPercentFromTop, const float fPercentFromLeft )
{
	PositionFromLeft( fPercentFromTop, fPercentFromLeft, UYA_CENTER, UXA_LEFT );
}

void GUIAnimation::PositionFromLeft( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_LEFT;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_CENTER;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromLeft;
	m_tAnchorInfo.m_fOffsetY = fPercentFromTop;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromTop( const float fPercentFromTop, const float fPercentFromLeft )
{
	PositionFromTop( fPercentFromTop, fPercentFromLeft, UYA_TOP, UXA_CENTER );
}

void GUIAnimation::PositionFromTop( const float fPercentFromTop, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_CENTER;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_TOP;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromLeft;
	m_tAnchorInfo.m_fOffsetY = fPercentFromTop;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromBottom( const float fPercentFromBottom, const float fPercentFromLeft )
{
	PositionFromBottom( fPercentFromBottom, fPercentFromLeft, UYA_BOTTOM, UXA_CENTER );
}

void GUIAnimation::PositionFromBottom( const float fPercentFromBottom, const float fPercentFromLeft, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_CENTER;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_BOTTOM;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromLeft;
	m_tAnchorInfo.m_fOffsetY = fPercentFromBottom;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::PositionFromRight( const float fPercentFromTop, const float fPercentFromRight )
{
	PositionFromRight( fPercentFromTop, fPercentFromRight, UYA_CENTER, UXA_RIGHT );
}

void GUIAnimation::PositionFromRight( const float fPercentFromTop, const float fPercentFromRight, const eUIyAnchor eYAnchor, const eUIxAnchor eXAnchor )
{
	// Update anchor information
	m_tAnchorInfo.m_eParentUIxAnchor = UXA_RIGHT;
	m_tAnchorInfo.m_eParentUIyAnchor = UYA_CENTER;
	m_tAnchorInfo.m_eUIxAnchor = eXAnchor;
	m_tAnchorInfo.m_eUIyAnchor = eYAnchor;
	m_tAnchorInfo.m_fOffsetX = fPercentFromRight;
	m_tAnchorInfo.m_fOffsetY = fPercentFromTop;
	m_tAnchorInfo.m_eUIPrecision = UIP_PERCENTAGE;

	// Refresh position
	RefreshPosition();
}

void GUIAnimation::RefreshPosition()
{
	m_bTouchAreaIsDirty = true;

	// Get parent anchor position
	hkvVec2 v2Position = ParentAnchorPosition();

	// Add position offset
	if ( m_tAnchorInfo.m_eUIPrecision == UIP_PERCENTAGE )
	{
		v2Position.x += UIRelative::XPercentFrom( m_tAnchorInfo.m_eUIxAnchor, ParentWidth(), m_tAnchorInfo.m_fOffsetX );
		v2Position.y += UIRelative::YPercentFrom( m_tAnchorInfo.m_eUIyAnchor, ParentHeight(), m_tAnchorInfo.m_fOffsetY );
	}
	else
	{

	}

	// Adjust for anchor offset
	v2Position.x -= UIRelative::XAnchorAdjustment( m_tAnchorInfo.m_eUIxAnchor, m_tTouchArea.m_fW, m_tAnchorInfo.m_eOriginUIxAnchor );
	v2Position.y += UIRelative::YAnchorAdjustment( m_tAnchorInfo.m_eUIyAnchor, m_tTouchArea.m_fH, m_tAnchorInfo.m_eOriginUIyAnchor );

	// Set new position
	m_spTexture->SetPos( v2Position.x, v2Position.y );
}

hkvVec2 GUIAnimation::ParentAnchorPosition() const
{
	hkvVec2 v2Position;
	float fWidth, fHeight;
	eUIxAnchor eOriginUIxAnchor = UXA_LEFT;
	eUIyAnchor eOriginUIyAnchor = UYA_TOP;
	// Determine correct parent values
	if ( !m_tAnchorInfo.m_pParent )
	{
		v2Position = hkvVec2::ZeroVector();
		fWidth = static_cast<float>(Vision::Video.GetXRes());
		fHeight = static_cast<float>(Vision::Video.GetYRes());
	}
	else
	{
		m_tAnchorInfo.m_pParent->GetPosition( v2Position.x, v2Position.y );
		m_tAnchorInfo.m_pParent->GetSize( fWidth, fHeight );
		eOriginUIxAnchor = m_tAnchorInfo.m_pParent->GetAnchorInfo().m_eOriginUIxAnchor;
		eOriginUIyAnchor = m_tAnchorInfo.m_pParent->GetAnchorInfo().m_eOriginUIyAnchor;
	}

	// Adjust anchor offset
	v2Position.x += UIRelative::XAnchorAdjustment( m_tAnchorInfo.m_eParentUIxAnchor, fWidth, eOriginUIxAnchor );
	v2Position.y -= UIRelative::YAnchorAdjustment( m_tAnchorInfo.m_eParentUIyAnchor, fHeight, eOriginUIyAnchor );

	return v2Position;
}

float GUIAnimation::ParentWidth() const
{
	return ( !m_tAnchorInfo.m_pParent ) ? Vision::Video.GetXRes() : m_tAnchorInfo.m_pParent->GetSize().x;
}

float GUIAnimation::ParentHeight() const
{
	return ( !m_tAnchorInfo.m_pParent ) ? Vision::Video.GetYRes() : m_tAnchorInfo.m_pParent->GetSize().y;
}

void GUIAnimation::RefreshTouchArea()
{
	// Update parent position if exists and is dirty
	if ( m_tAnchorInfo.m_pParent && m_tAnchorInfo.m_pParent->IsTouchAreaDirty() ) m_tAnchorInfo.m_pParent->DemandRefreshTouchArea();
	if ( m_tAnchorInfo.m_pParent && m_bTouchAreaIsDirty ) RefreshPosition(); // FIXME: Not optimal, elements with parent are refreshed twice

	// Get tex pos and size
	float fTexPosX, fTexPosY, fTexW, fTexH;
	m_spTexture->GetPos( fTexPosX, fTexPosY );
	m_spTexture->GetTargetSize( fTexW, fTexH );
	// Update touch area
	m_tTouchArea.Set( fTexPosX, fTexPosY, fTexW, fTexH );
	// Update trigger area
	GUIAnimation::SetMapTouchArea( m_tTouchArea.m_fX, m_tTouchArea.m_fY, m_tTouchArea.m_fW, m_tTouchArea.m_fH, m_eID );
	// Re-center the rotation anchor
	m_spTexture->SetRotationCenter( m_tTouchArea.m_fW / 2.f, m_tTouchArea.m_fH / 2.f );

	// Update position
	RefreshPosition();
	
	// FIXME:
	//m_bTouchAreaIsDirty = false;
}


void GUIAnimation::SetMapTouchArea( const float fX, const float fY, const float fWidth, const float fHeight, const eGUIAnimID eID )
{
	/********************************************************************************************* MapTriggers dont working
	float const fDepth = -925.0f;
	const VRectanglef rect( fX, fY, fX + fWidth, fY + fHeight );

	if ( eID == GAI_BTN_PLAY_THE_GAME ) 
		float a = 2.f;

	#if defined(_VISION_MOBILE)
	VTouchArea* pTouchArea = new VTouchArea( VInputManager::GetTouchScreen(), rect, fDepth );
	GUIAnimationManager::Instance().GetInputMap()->MapTrigger( eID, pTouchArea, CT_TOUCH_ANY );
	#endif
	*********************************************************************************************/
}

void GUIAnimation::TryPlayOnEasingCompleteSound() const 
{
	if ( m_eOnEasingCompleteSound != eNoSound ) SoundManager::Instance()->PlaySound( m_eOnEasingCompleteSound, Vision::Camera.GetMainCamera()->GetPosition(), false );
}

void GUIAnimation::TryPlayOnEasingStartSound() const 
{
	if ( m_eOnEasingStartSound != eNoSound ) SoundManager::Instance()->PlaySound( m_eOnEasingStartSound, Vision::Camera.GetMainCamera()->GetPosition(), false );
}

EasingAnimation* GUIAnimation::AlphaTo( const float fStartTimeOut, const float fDuration, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return To( fStartTimeOut, fDuration, GAP_ALPHA, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::AlphaFrom( const float fStartTimeOut, const float fDuration, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return From( fStartTimeOut, fDuration, GAP_ALPHA, fStart, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::AlphaFromTo( const float fStartTimeOut, const float fDuration, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return FromTo( fStartTimeOut, fDuration, GAP_ALPHA, fStart, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::ColorTo( const float fStartTimeOut, const float fDuration, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return To( fStartTimeOut, fDuration, GAP_COLOR, tTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::ColorFrom( const float fStartTimeOut, const float fDuration, const VColorRef& tStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return From( fStartTimeOut, fDuration, GAP_COLOR, tStart, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::ColorFromTo( const float fStartTimeOut, const float fDuration, const VColorRef& tStart, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return FromTo( fStartTimeOut, fDuration, GAP_COLOR, tStart, tTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::AnglesTo( const float fStartTimeOut, const float fDuration, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return To( fStartTimeOut, fDuration, GAP_ANGLES, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::AnglesFrom( const float fStartTimeOut, const float fDuration, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return From( fStartTimeOut, fDuration, GAP_ANGLES, fStart, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::AnglesFromTo( const float fStartTimeOut, const float fDuration, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return FromTo( fStartTimeOut, fDuration, GAP_ANGLES, fStart, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::ScaleTo( const float fStartTimeOut, const float fDuration, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return To( fStartTimeOut, fDuration, GAP_SCALE, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::ScaleFrom( const float fStartTimeOut, const float fDuration, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return From( fStartTimeOut, fDuration, GAP_SCALE, fStart, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::ScaleFromTo( const float fStartTimeOut, const float fDuration, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return FromTo( fStartTimeOut, fDuration, GAP_SCALE, fStart, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

/*
	Position given in relative (normalized) measures from top left anchor
*/
EasingAnimation* GUIAnimation::PositionTo( const float fStartTimeOut, const float fDuration, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return To( fStartTimeOut, fDuration, GAP_POSITION, v2Target, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

/*
	Position given in relative (normalized) measures from top left anchor
*/
EasingAnimation* GUIAnimation::PositionFrom( const float fStartTimeOut, const float fDuration, const hkvVec2& v2Start, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return From( fStartTimeOut, fDuration, GAP_POSITION, v2Start, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

/*
	Position given in relative (normalized) measures from top left anchor
*/
EasingAnimation* GUIAnimation::PositionFromTo( const float fStartTimeOut, const float fDuration, const hkvVec2& v2Start, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return FromTo( fStartTimeOut, fDuration, GAP_POSITION, v2Start, v2Target, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::To( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( true, fStartTimeOut, fDuration, eProperty, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::To( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( true, fStartTimeOut, fDuration, eProperty, tTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::To( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( true, fStartTimeOut, fDuration, eProperty, v2Target, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::From( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( false, fStartTimeOut, fDuration, eProperty, fStart, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::From( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tStart, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( false, fStartTimeOut, fDuration, eProperty, tStart, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::From( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Start, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( false, fStartTimeOut, fDuration, eProperty, v2Start, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::FromTo( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( fStartTimeOut, fDuration, eProperty, fStart, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::FromTo( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tStart, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( fStartTimeOut, fDuration, eProperty, tStart, tTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::FromTo( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Start, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	return Animate( fStartTimeOut, fDuration, eProperty, v2Start, v2Target, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::Animate( const bool bAnimateTo, const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	// Play sound event
	TryPlayOnEasingStartSound();

	float fCurrent = 0.0f;

	// Grab the current value
	switch ( eProperty ) 
	{
	case GAP_ALPHA:
		{
			fCurrent = m_spTexture->GetColor().a;
			break;
		}
	case GAP_ANGLES:
		{
			fCurrent = m_spTexture->GetRotationAngle();
			break;
		}
	case GAP_SCALE:
		{
			fCurrent = GetSize().x / m_fInitWidth;
			break;
		}
	default:
		break;
	}

	float fStart = ( bAnimateTo ) ? fCurrent : fTarget;

	// If we are doing a 'from', the target is our current position
	float _fTarget = fTarget;
	if ( !bAnimateTo ) _fTarget = fCurrent;

	return Animate( fStartTimeOut, fDuration, eProperty, fStart, _fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::Animate( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	OverrideAnimTypeIfExists( eProperty );

	// Set the start value
	switch ( eProperty ) 
	{
	case GAP_ALPHA:
		{
			if ( m_spTexture ) 
			{
				VColorRef tStartColor = m_spTexture->GetColor();
				tStartColor.a = static_cast<UBYTE>(fStart);
				m_spTexture->SetColor( tStartColor );
			}
			break;
		}
	case GAP_ANGLES:
		{
			SetRotationAngle( fStart );
			break;
		}
	case GAP_SCALE:
		{
			SetScale( fStart, fStart );
			break;
		}
	default:
		break;
	}

	m_bActiveEaseAnim = true;
	EasingAnimation* pEasingAnim = new EasingAnimation( this, fStartTimeOut, fDuration, eProperty, fStart, fTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
	m_aEasingAnims[ m_aEasingAnims.GetFreePos() ] = pEasingAnim;

	return pEasingAnim;
}

EasingAnimation* GUIAnimation::Animate( const bool bAnimateTo, const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	// Play sound event
	TryPlayOnEasingStartSound();

	// Grab the current value
	const VColorRef& tCurrent = m_spTexture->GetColor();
	const VColorRef& tStart = ( bAnimateTo ) ? tCurrent : tTarget;

	// If we are doing a 'from', the target is our current position
	VColorRef _tTarget = const_cast<VColorRef&>(tTarget);
	if ( !bAnimateTo ) _tTarget = tCurrent;

	return Animate( fStartTimeOut, fDuration, eProperty, tStart, _tTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::Animate( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const VColorRef& tStart, const VColorRef& tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	OverrideAnimTypeIfExists( eProperty );

	// Set the start value
	if ( m_spTexture ) m_spTexture->SetColor( tStart );

	m_bActiveEaseAnim = true;
	EasingAnimation* pEasingAnim = new EasingAnimation( this, fStartTimeOut, fDuration, eProperty, tStart, tTarget, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
	m_aEasingAnims[ m_aEasingAnims.GetFreePos() ] = pEasingAnim;

	return pEasingAnim;
}

EasingAnimation* GUIAnimation::Animate( const bool bAnimateTo, const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	// Play sound event
	TryPlayOnEasingStartSound();

	hkvVec2 v2Current( 0.f, 0.f );
	// Grab the current value
	GetRelativePostition( v2Current.x, v2Current.y );
	const hkvVec2& v2Start = ( bAnimateTo ) ? v2Current : v2Target;

	// If we are doing a 'from', the target is our current position
	hkvVec2 _v2Target = v2Target;
	if ( !bAnimateTo ) _v2Target = v2Current;

	return Animate( fStartTimeOut, fDuration, eProperty, v2Start, _v2Target, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
}

EasingAnimation* GUIAnimation::Animate( const float fStartTimeOut, const float fDuration, const eGUIAnimProperty eProperty, const hkvVec2& v2Start, const hkvVec2& v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfOnComplete, const bool bAffectedByTimeScale ) 
{
	OverrideAnimTypeIfExists( eProperty );

	// Set the start value
	PositionFromTopLeft( v2Start.y, v2Start.x );

	m_bActiveEaseAnim = true;
	EasingAnimation* pEasingAnim = new EasingAnimation( this, fStartTimeOut, fDuration, eProperty, v2Start, v2Target, pfEaseMethod, pfOnComplete, bAffectedByTimeScale );
	m_aEasingAnims[ m_aEasingAnims.GetFreePos() ] = pEasingAnim;

	return pEasingAnim;
}


// UIRelative member functions
float UIRelative::XPercentFrom( const eUIxAnchor eAnchor, const float fWidth, const float fPercentOffset ) 
{
	// Get initial offset
	float fOffset = fWidth * fPercentOffset;

	// If anchor is right the offset is flipped
	if ( eAnchor == UXA_RIGHT )
	{
		fOffset = -fOffset;
	}
	return fOffset;
}

float UIRelative::YPercentFrom( const eUIyAnchor eAnchor, const float fHeight, const float fPercentOffset ) 
{
	// Get initial offset
	float fOffset = fHeight * fPercentOffset;

	// If anchor is bottom the offset is flipped
	if ( eAnchor == UYA_BOTTOM )
	{
		fOffset = -fOffset;
	}
	return fOffset;
}

float UIRelative::XPercentTo( const eUIxAnchor eAnchor, const float fWidth, const float fOffset ) 
{
	if ( fWidth == 0.f ) return 0.f;

	// Get initial percentage
	float fPercentOffset = fOffset / fWidth;

	// If anchor is right the percentage is flipped
	if ( eAnchor == UXA_RIGHT )
	{
		fPercentOffset = -fPercentOffset;
	}
	return fPercentOffset;
}

float UIRelative::YPercentTo( const eUIyAnchor eAnchor, const float fHeight, const float fOffset ) 
{
	if ( fHeight == 0.f ) return 0.f;

	// Get initial percentage
	float fPercentOffset = fOffset / fHeight;

	// If anchor is bottom the percentage is flipped
	if ( eAnchor == UYA_BOTTOM )
	{
		fPercentOffset = -fPercentOffset;
	}
	return fPercentOffset;
}


float UIRelative::XAnchorAdjustment( const eUIxAnchor eAnchor, const float fWidth, const eUIxAnchor eOriginAnchor ) 
{
	float fAdjustment = 0.f;
	switch ( eAnchor )
	{
	case UXA_LEFT:

		if ( eOriginAnchor == UXA_CENTER )
		{
			fAdjustment -= fWidth / 2.f;
		}
		else if ( eOriginAnchor == UXA_RIGHT )
		{
			fAdjustment -= fWidth;
		}
		break;
	case UXA_RIGHT:

		if ( eOriginAnchor == UXA_LEFT )
		{
			fAdjustment += fWidth;
		}
		else if ( eOriginAnchor == UXA_CENTER )
		{
			fAdjustment += fWidth / 2.f;
		}
		break;
	case UXA_CENTER:

		if ( eOriginAnchor == UXA_LEFT )
		{
			fAdjustment += fWidth / 2.f;
		}
		else if ( eOriginAnchor == UXA_RIGHT )
		{
			fAdjustment -= fWidth / 2.f;
		}
		break;
	default:
		break;
	}

	return fAdjustment;
}

float UIRelative::YAnchorAdjustment( const eUIyAnchor eAnchor, const float fHeight, const eUIyAnchor eOriginAnchor ) 
{
	float fAdjustment = 0.f;
	switch ( eAnchor )
	{
	case UYA_TOP:

		if ( eOriginAnchor == UYA_CENTER )
		{
			fAdjustment -= fHeight / 2.f;
		}
		else if ( eOriginAnchor == UYA_BOTTOM )
		{
			fAdjustment -= fHeight;
		}
		break;
	case UYA_BOTTOM:

		if ( eOriginAnchor == UYA_TOP )
		{
			fAdjustment -= fHeight;
		}
		else if ( eOriginAnchor == UYA_CENTER )
		{
			fAdjustment += fHeight / 2.f;
		}
		break;
	case UYA_CENTER:

		if ( eOriginAnchor == UYA_TOP )
		{
			fAdjustment -= fHeight / 2.f;
		}
		else if ( eOriginAnchor == UYA_BOTTOM )
		{
			fAdjustment += fHeight / 2.f;
		}
		break;
	default:
		break;
	}

	return fAdjustment;
}


// Easing algorithms functions
// Linear
float Easing::Linear::EaseIn( const float fT ) 
{
	return fT;
}

float Easing::Linear::EaseOut( const float fT ) 
{
	return fT;
}

float Easing::Linear::EaseInOut( const float fT ) 
{
	return fT;
}

// Quartic
float Easing::Quartic::EaseIn( const float fT ) 
{
	return hkvMath::pow( fT, 4.f );
}

float Easing::Quartic::EaseOut( const float fT ) 
{
	return ((hkvMath::pow( fT - 1.f, 4.f ) - 1.f ) * -1.f);
}

float Easing::Quartic::EaseInOut( const float fT ) 
{
	if ( fT <= 0.5f ) return EaseIn( fT * 2.f ) / 2.f;
	else return ( EaseOut( ( fT - 0.5f ) * 2.f ) / 2.f ) + 0.5f;
}

// Quintic
float Easing::Quintic::EaseIn( const float fT ) 
{
	return hkvMath::pow( fT, 5.f );
}

float Easing::Quintic::EaseOut( const float fT ) 
{
	return (hkvMath::pow( fT - 1.f, 5.f ) + 1.f);
}

float Easing::Quintic::EaseInOut( const float fT ) 
{
	if ( fT <= 0.5f ) return EaseIn( fT * 2.f ) / 2.f;
	else return ( EaseOut( ( fT - 0.5f ) * 2.f ) / 2.f ) + 0.5f;
}

// Sinusoidal
float Easing::Sinusoidal::EaseIn( const float fT ) 
{
	return hkvMath::sinRad( ( fT - 1.f ) * ( hkvMath::pi() / 2.f ) ) + 1.f;
}

float Easing::Sinusoidal::EaseOut( const float fT ) 
{
	return hkvMath::sinRad( fT * ( hkvMath::pi() / 2.f ) );
}

float Easing::Sinusoidal::EaseInOut( const float fT ) 
{
	if ( fT <= 0.5f ) return EaseIn( fT * 2.f ) / 2.f;
	else return ( EaseOut( ( fT - 0.5f ) * 2.f ) / 2.f ) + 0.5f;
}

// Exponential
float Easing::Exponential::EaseIn( const float fT ) 
{
	return hkvMath::pow( 2.f, 10.f * ( fT - 1.f ) );
}

float Easing::Exponential::EaseOut( const float fT ) 
{
	return 1.f - hkvMath::pow( 2.f, -10.f * fT );
}

float Easing::Exponential::EaseInOut( const float fT ) 
{
	if ( fT <= 0.5f ) return EaseIn( fT * 2.f ) / 2.f;
	else return ( EaseOut( ( fT - 0.5f ) * 2.f ) / 2.f ) + 0.5f;
}

// Circular
float Easing::Circular::EaseIn( const float fT ) 
{
	return (-1.f * hkvMath::sqrt( 1.f - fT * fT ) + 1.f);
}

float Easing::Circular::EaseOut( const float fT ) 
{
	return hkvMath::sqrt( 1.f - hkvMath::pow( fT - 1.f, 2.f ) );
}

float Easing::Circular::EaseInOut( const float fT ) 
{
	if ( fT <= 0.5f ) return EaseIn( fT * 2.f ) / 2.f;
	else return ( EaseOut( ( fT - 0.5f ) * 2.f ) / 2.f ) + 0.5f;
}

// Back
float Easing::Back::s_fS = 1.70158f;
float Easing::Back::s_fS2 = 1.70158f * 1.525f;

float Easing::Back::EaseIn( const float fT ) 
{
	return fT * fT * ( ( s_fS + 1.f ) * fT - 2.f );
}

float Easing::Back::EaseOut( const float _fT ) 
{
	float fT = _fT - 1.f;
	return ( fT * fT * ( ( s_fS + 1.f ) * fT + s_fS ) + 1.f );
}

float Easing::Back::EaseInOut( const float _fT ) 
{
	float fT = _fT * 2.f;
	if ( fT < 1.f ) 
	{
		return 0.5f * ( fT * fT * ( ( s_fS2 + 1.f ) * fT - s_fS2 ) );
	}
	else
	{
		fT -= 2.f;
		return 0.5f * ( fT * fT * ( ( s_fS2 + 1.f ) * fT - s_fS2 ) + 2.f );
	}
}

// Bounce
float Easing::Bounce::s_fB = 0.f;
float Easing::Bounce::s_fC = 1.f;
float Easing::Bounce::s_fD = 1.f;

float Easing::Bounce::EaseOut( const float _fT ) 
{
	float fT = _fT;
	if ( (fT /= s_fD) < (1.f / 2.75f) ) 
	{
		return s_fC * (7.5625f * fT * fT) + s_fB;
	}
	else if ( fT < (2.f / 2.75f) )
	{
		return s_fC * (7.5625f * (fT -= (1.5f / 2.75f)) * fT + 0.75f) + s_fB;
	}
	else if ( fT < (2.5f / 2.75f) )
	{
		return s_fC * (7.5625f * (fT -= (2.25f / 2.75f)) * fT + 0.9375f) + s_fB;
	}
	else
	{
		return s_fC * (7.5625f * (fT -= (2.625f / 2.75f)) * fT + 0.984375f) + s_fB;
	}
}

float Easing::Bounce::EaseIn( const float fT ) 
{
	return s_fC - EaseOut( s_fD - fT ) + s_fB;
}

float Easing::Bounce::EaseInOut( const float fT ) 
{
	if ( fT < s_fD / 2.f ) return EaseIn( fT * 2.f ) * 0.5f + s_fB;
	else return EaseOut( fT * 2.f - s_fD ) * 0.5f + s_fC * 0.5f + s_fB;
}

// Elastic
float Easing::Elastic::EaseIn( const float fT ) 
{
	return hkvMath::sinRad( 13.f * (hkvMath::pi() / 2.f) * fT ) * hkvMath::pow( 2.f, 10.f * (fT - 1.f) );
}

float Easing::Elastic::EaseOut( const float fT ) 
{
	return hkvMath::sinRad( -13.f * (hkvMath::pi() / 2.f) * (fT + 1.f) ) * hkvMath::pow( 2.f, -10.f * fT ) + 1.f;
}

float Easing::Elastic::EaseInOut( const float fT ) 
{
	if ( fT < 0.5f ) return 0.5f * hkvMath::sinRad( 13.f * (hkvMath::pi() / 2.f) * (2.f * fT) ) * hkvMath::pow( 2.f, 10.f * ((2.f * fT) - 1.f));
	else return 0.5f * (hkvMath::sinRad( -13.f * (hkvMath::pi() / 2.f) * ((2.f * fT - 1.f) + 1.f)) * hkvMath::pow( 2.f, -10.f * (2.f * fT - 1.f)) + 2.f);
}

// EasingAnimation member functions
EasingAnimation::EasingAnimation( GUIAnimation* pGUIObject, const float fStartTimeOut, const float fDuration, eGUIAnimProperty eProperty, const hkvVec2 v2Start, const hkvVec2 v2Target, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfCallback, const bool bAffectedByTimeScale )
{
	m_pGUIObject = pGUIObject;
	m_fDuration = fDuration;
	m_eAnimProperty = eProperty;
	m_pfCallback = pfCallback;
	m_pfEase = pfEaseMethod;
	m_bAffectedByTimeScale = bAffectedByTimeScale;
	m_v2Start = v2Start;
	m_v2Target = v2Target;
	m_bAutoreverse = false;
	// Also init other property variables
	m_fStart = 0.f; m_fTarget = 0.f;
	m_tStart.SetRGBA( 0, 0, 0, 0 ); m_tTarget.SetRGBA( 0, 0, 0, 0 );

	m_bFinished = false;
	m_bRunning = true;
	// Store out start time
	m_fStartTime = bAffectedByTimeScale ? Vision::GetTimer()->GetTime() : Vision::GetTimer()->GetCurrentTime();
	m_fStartTime += fStartTimeOut;
}

EasingAnimation::EasingAnimation( GUIAnimation* pGUIObject, const float fStartTimeOut, const float fDuration, eGUIAnimProperty eProperty, const float fStart, const float fTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfCallback, const bool bAffectedByTimeScale )
{
	m_pGUIObject = pGUIObject;
	m_fDuration = fDuration;
	m_eAnimProperty = eProperty;
	m_pfCallback = pfCallback;
	m_pfEase = pfEaseMethod;
	m_bAffectedByTimeScale = bAffectedByTimeScale;
	m_fStart = fStart;
	m_fTarget = fTarget;
	m_bAutoreverse = false;
	// Also init other property variables
	m_v2Start = hkvVec2::ZeroVector(); m_v2Target = hkvVec2::ZeroVector();
	m_tStart.SetRGBA( 0, 0, 0, 0 ); m_tTarget.SetRGBA( 0, 0, 0, 0 );

	m_bFinished = false;
	m_bRunning = true;
	// Store out start time
	m_fStartTime = bAffectedByTimeScale ? Vision::GetTimer()->GetTime() : Vision::GetTimer()->GetCurrentTime();
	m_fStartTime += fStartTimeOut;
}

EasingAnimation::EasingAnimation( GUIAnimation* pGUIObject, const float fStartTimeOut, const float fDuration, eGUIAnimProperty eProperty, const VColorRef tStart, const VColorRef tTarget, const pfEase pfEaseMethod, const pfGUIEasingAnimationCallback pfCallback, const bool bAffectedByTimeScale )
{
	m_pGUIObject = pGUIObject;
	m_fDuration = fDuration;
	m_eAnimProperty = eProperty;
	m_pfCallback = pfCallback;
	m_pfEase = pfEaseMethod;
	m_bAffectedByTimeScale = bAffectedByTimeScale;
	m_tStart = tStart;
	m_tTarget = tTarget;
	m_bAutoreverse = false;
	// Also init other property variables
	m_fStart = 0.f; m_fTarget = 0.f;
	m_v2Start = hkvVec2::ZeroVector(); m_v2Target = hkvVec2::ZeroVector();

	m_bFinished = false;
	m_bRunning = true;
	// Store out start time
	m_fStartTime = bAffectedByTimeScale ? Vision::GetTimer()->GetTime() : Vision::GetTimer()->GetCurrentTime();
	m_fStartTime += fStartTimeOut;
}

EasingAnimation::~EasingAnimation()
{
	m_pfCallback = 0;
	m_pGUIObject = 0;
	m_pfEase = 0;
}

void EasingAnimation::Update( const float fDeltaTime )
{
	if ( m_bRunning ) 
	{
		float fCurrentTime = m_bAffectedByTimeScale ? Vision::GetTimer()->GetTime() : Vision::GetTimer()->GetCurrentTime();
		if ( m_fStartTime > fCurrentTime ) return;
		// Get our easing position
		float fEasePos = hkvMath::clamp( (fCurrentTime - m_fStartTime) / m_fDuration, 0.f, 1.f );
		fEasePos = m_pfEase( fEasePos );

		// Set the proper property
		switch ( m_eAnimProperty ) 
		{
		case GAP_POSITION:
			{
				hkvVec2 v2Pos( 0.f , 0.f );
				v2Pos.setInterpolate( m_v2Start, m_v2Target, fEasePos );
				m_pGUIObject->PositionFromTopLeft( v2Pos.y, v2Pos.x );
				break;
			}
		case GAP_SCALE:
			{
				float fScale = hkvMath::interpolate( m_fStart, m_fTarget, fEasePos );
				m_pGUIObject->SetScale( fScale, fScale );
				break;
			}
		case GAP_ANGLES:
			{
				m_pGUIObject->SetRotationAngle( hkvMath::interpolate( m_fStart, m_fTarget, fEasePos ) );
				break;
			}
		case GAP_ALPHA:
			{
				VColorRef tColor = m_pGUIObject->GetTexture()->GetColor();
				tColor.a = static_cast<UBYTE>(hkvMath::interpolate( m_fStart, m_fTarget, fEasePos ));
				m_pGUIObject->SetColor( tColor );
				break;
			}
		case GAP_COLOR:
			{
				VColorRef tColor( 0, 0, 0, 0 );
				tColor.Lerp( m_tStart, m_tTarget, fEasePos );
				tColor.a = m_pGUIObject->GetTexture()->GetColor().a; // Set current alpha value
				m_pGUIObject->SetColor( tColor );
				break;
			}
		default:
			break;
		}
	
		// See if we are done with our animation yet
		if ( (m_fStartTime + m_fDuration) <= fCurrentTime ) 
		{
			// If we are set to autoreverse, flip around a few variables and continue
			if ( m_bAutoreverse ) 
			{
				m_bAutoreverse = false; // Make sure this only happens once

				// Flip out start and target
				hkvVec2 v2Temp = m_v2Start;
				m_v2Start = m_v2Target;
				m_v2Target = v2Temp;
				// Flip alpha variables
				float fTemp = m_fStart;
				m_fStart = m_fTarget;
				m_fTarget = fTemp;
				// Flip color variables
				VColorRef tTemp = m_tStart;
				m_tStart = m_tTarget;
				m_tTarget = tTemp;
				// Reset the start time
				m_fStartTime = fCurrentTime;
			}
			else
			{
				m_bRunning = false;
				m_bFinished = true;
			}
		}
	} // End while

	if ( m_bFinished ) OnFinished();
}

void EasingAnimation::OnFinished()
{
	m_pGUIObject->TryPlayOnEasingCompleteSound();

	if ( m_pfCallback )
		m_pfCallback( m_pGUIObject, m_eAnimProperty );
}
