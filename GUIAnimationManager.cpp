#include "CutshumotoPluginPCH.h"
#include "GUIAnimationManager.h"
#include "../CutshumotoPlugin/CutshumotoUtilities.h"
#include <algorithm>


namespace 
{
	bool CompareAnimationOrder( GUIAnimation* pAnim0, GUIAnimation* pAnim1 ) { return pAnim0->GetTexture()->GetOrder() < pAnim1->GetTexture()->GetOrder(); }
}

GUIAnimationManager* GUIAnimationManager::s_pInstance = 0;

GUIAnimationManager::GUIAnimationManager()
{
	m_aAnimations.Init(0);
	m_pInputMap = new VInputMap( GAI_COUNT, 4 );
	m_bIsValid = false;
	m_sHDExtension = "2x";
	m_bIsHD = false;
	m_pTouchHandler = 0;
	m_bAnimsArrayIsDirty = true;

	if ( AUTO_LOAD_HD_TEX )
	{
		float fScreenW, fScreenH;
		fScreenW = static_cast<float>(Vision::Video.GetXRes());
		fScreenH = static_cast<float>(Vision::Video.GetYRes());
		if ( fScreenW >= MAX_W_OR_H_HD || fScreenH >= MAX_W_OR_H_HD ) 
		{
			m_bIsHD = true;
			m_sHDExtension = "4x";
		} 
		else if ( fScreenW >= MAX_W_OR_H_SD || fScreenH >= MAX_W_OR_H_SD ) 
		{
			m_bIsHD = true;
		}
	}
}

GUIAnimationManager::~GUIAnimationManager()
{
	for ( unsigned int i = 0; i < m_aAnimations.GetValidSize(); i++ ) 
	{
		if ( m_aAnimations[i] ) 
		{
			delete m_aAnimations[i];
			m_aAnimations.Remove(i);
		}
	}
	m_aAnimations.Reset();
	m_hDecodedTexturePackerJSON.clear();
	// Free TriggerMaps
	if ( m_pInputMap ) 
	{
		m_pInputMap->Clear();
		delete m_pInputMap;
	}
	m_pTouchHandler = 0;
}

GUIAnimation* GUIAnimationManager::CreateAnimation( const std::string& sSrcTexFilepathWithoutExtension, const std::string& _sFilename, const int iFirstFrame, const int iLastFrame, const int iNumFrames, const eGUIAnimID eID, const GUIAnimation::eGUIAnimType eType )
{
	if ( iNumFrames > 0 && !m_bIsValid ) return 0;

	std::string sSrcTexFilepath = sSrcTexFilepathWithoutExtension;
	std::string sFilename = _sFilename;
	if ( m_bIsHD )
	{ // Load hd textures
		if ( iNumFrames == 0 )
		{ // Standalone texture
			int iExtensionDotPos = _sFilename.rfind( "." );
			std::string sExtension = _sFilename.substr( iExtensionDotPos + 1 );
			std::string sBaseFilename = _sFilename.substr( 0, iExtensionDotPos );
			sFilename = sBaseFilename + m_sHDExtension;
			sFilename += "." + sExtension; // Filename with hd extension and file format
		}
		else
		{ // Texture atlas
			sSrcTexFilepath += m_sHDExtension + TPTEXFILE_EXTENSION; // Filepath with hd extension and file format
		}
	}

	GUIAnimation* pNewAnim = GUIAnimation::Create( sSrcTexFilepath, sFilename, iFirstFrame, iLastFrame, iNumFrames, eID, eType );
	AddAnimation( pNewAnim );

	return pNewAnim;
}

void GUIAnimationManager::AddAnimation( GUIAnimation* guiAnimation )
{
	m_aAnimations[ m_aAnimations.GetFreePos() ] = guiAnimation;
}

void GUIAnimationManager::RemoveAnimation( GUIAnimation* guiAnimation )
{
	for ( unsigned int i = 0; i < m_aAnimations.GetValidSize(); i++ )
		if ( m_aAnimations[i] == guiAnimation ) m_aAnimations.Remove(i);
	m_aAnimations.Pack();
}

GUIAnimation* GUIAnimationManager::GetAnimation( unsigned int eAnimID )
{
	for ( unsigned int i = 0; i < m_aAnimations.GetValidSize(); i++ )
		if ( m_aAnimations[i]->GetAnimID() == eAnimID ) return m_aAnimations[i];
	return 0;
 }

void GUIAnimationManager::Update( float fDeltaTime )
{
	if ( m_bAnimsArrayIsDirty ) 
	{
		m_bAnimsArrayIsDirty = false;
		// Update order of elements in array
		unsigned int uiAnimationsCount = m_aAnimations.GetValidSize();
		GUIAnimation** ppEndIterator = m_aAnimations.GetDataPtr();
		ppEndIterator += uiAnimationsCount;
		// Order guianimations by Order attribute in ASC way
		std::sort( m_aAnimations.GetDataPtr(), ppEndIterator, CompareAnimationOrder );
	}

	for ( unsigned int i = 0; i < m_aAnimations.GetValidSize(); i++ )
	{
		if ( !m_aAnimations[i]->IsVisible() ) continue;
		// Update input event
		if ( m_pTouchHandler ) 
			if ( m_pTouchHandler->IsTouchable() ) m_pTouchHandler->UpdateInput(); // The control already being touched
			else m_pTouchHandler = 0; // The control cant handle input events nevermore since is untouchable
		else 
			if ( m_aAnimations[i]->IsTouchable() ) m_aAnimations[i]->UpdateInput(); // Looking for touch handler
		// Update position and animation
		m_aAnimations[i]->Update( fDeltaTime );
	}
}

bool GUIAnimationManager::LoadTexturePackerJSON( const std::string& sFilenameWithoutExtension, const std::string& sPath )
{
	std::string sFilename = sFilenameWithoutExtension;
	if ( m_bIsHD )
		sFilename += m_sHDExtension;
	sFilename += ".json";

	char pcFileContent[MAX_TPJSONFILE_SIZE];
	bool bFileLoaded = CutshumotoUtilities::ReadFile( pcFileContent, sFilename.c_str(), sPath.c_str() );

	if ( !bFileLoaded ) return false;

	rapidjson::Document jsonDoc;
	jsonDoc.Parse<0>( pcFileContent );

	const rapidjson::Value& frames = jsonDoc["frames"];
	if ( !frames.IsArray() ) return false;

	for ( unsigned int i = 0; i < frames.Size(); i++ )
	{
		if ( frames[i].HasMember( "filename" ) && frames[i].HasMember( "frame" ) )
		{
			m_hDecodedTexturePackerJSON[frames[i]["filename"].GetString()] = GUIAnimation::FrameRect( 
				static_cast<float>(frames[i]["frame"]["x"].GetInt()), 
				static_cast<float>(frames[i]["frame"]["y"].GetInt()), 
				static_cast<float>(frames[i]["frame"]["w"].GetInt()), 
				static_cast<float>(frames[i]["frame"]["h"].GetInt()) );
		}
	}
	m_bIsValid = true;
	return true;
}
