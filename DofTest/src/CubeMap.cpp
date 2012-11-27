#include "CubeMap.h"

using namespace std;
using namespace ci;

namespace mndl { namespace gl {

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetGLCubeMap
template<typename T>
class ImageTargetGLCubeMap : public ImageTarget {
 public:
    static shared_ptr<ImageTargetGLCubeMap> createRef( const CubeMap *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha );
    ~ImageTargetGLCubeMap();

    virtual bool    hasAlpha() const { return mHasAlpha; }
    virtual void*   getRowPointer( int32_t row ) { return mData + row * mRowInc; }

    const void*     getData() const { return mData; }

 private:
    ImageTargetGLCubeMap( const CubeMap *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha );
    const CubeMap  *mTexture;
    bool                mIsGray;
    bool                mHasAlpha;
    uint8_t             mPixelInc;
    T                   *mData;
    int                 mRowInc;
};

/////////////////////////////////////////////////////////////////////////////////
// CubeMap
CubeMap::CubeMap( const ImageSourceRef &pos_x, const ImageSourceRef &pos_y,
		const ImageSourceRef &pos_z, const ImageSourceRef &neg_x,
		const ImageSourceRef &neg_y, const ImageSourceRef &neg_z,
		const ci::gl::Texture::Format &format ) :
	mObj( shared_ptr< Obj >( new Obj() ) )
{
	mObj->mTarget = GL_TEXTURE_CUBE_MAP;
	mObj->mWidth = pos_x->getWidth();
	mObj->mHeight = pos_x->getHeight();

#if defined( CINDER_MAC ) || defined( CINDER_LINUX )
	bool supportsTextureFloat = ci::gl::isExtensionAvailable( "GL_ARB_texture_float" );
#elif defined( CINDER_MSW )
	bool supportsTextureFloat = GLEE_ARB_texture_float != 0;
#endif

	// create a texture object
	glGenTextures( 1, &mObj->mTextureID );
	glBindTexture( mObj->mTarget, mObj->mTextureID );

	glTexParameteri( mObj->mTarget, GL_TEXTURE_WRAP_S, format.getWrapS() );
	glTexParameteri( mObj->mTarget, GL_TEXTURE_WRAP_T, format.getWrapT() );
	glTexParameteri( mObj->mTarget, GL_TEXTURE_MIN_FILTER, format.getMinFilter() );
	glTexParameteri( mObj->mTarget, GL_TEXTURE_MAG_FILTER, format.getMagFilter() );

	if( format.hasMipmapping() )
		glTexParameteri( mObj->mTarget, GL_GENERATE_MIPMAP, GL_TRUE );
	if( mObj->mTarget == GL_TEXTURE_2D ) {
		mObj->mMaxU = mObj->mMaxV = 1.0f;
	}
	else {
		mObj->mMaxU = (float)mObj->mWidth;
		mObj->mMaxV = (float)mObj->mHeight;
	}

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	const ImageSourceRef imageSources[] = { pos_x, pos_y, pos_z, neg_x, neg_y, neg_z };
	const GLenum targets[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
								GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
								GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
								GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
								GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
								GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB };

	for( int i = 0; i < 6; i++ ) {
		// Set the internal format based on the image's color space
		if( format.isAutoInternalFormat() ) {
			switch( imageSources[ i ]->getColorModel() ) {
#if ! defined( CINDER_GLES )
				case ImageIo::CM_RGB:
					if( imageSources[ i ]->getDataType() == ImageIo::UINT8 )
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA8 : GL_RGB8;
					else if( imageSources[ i ]->getDataType() == ImageIo::UINT16 )
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA16 : GL_RGB16;
					else if( imageSources[ i ]->getDataType() == ImageIo::FLOAT32 && supportsTextureFloat )
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA32F_ARB : GL_RGB32F_ARB;
					else
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA : GL_RGB;
					break;
				case ImageIo::CM_GRAY:
					if( imageSources[ i ]->getDataType() == ImageIo::UINT8 )
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE8;
					else if( imageSources[ i ]->getDataType() == ImageIo::UINT16 )
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_LUMINANCE16_ALPHA16 : GL_LUMINANCE16;
					else if( imageSources[ i ]->getDataType() == ImageIo::FLOAT32 && supportsTextureFloat )
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_LUMINANCE_ALPHA32F_ARB : GL_LUMINANCE32F_ARB;
					else
						mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
					break;
#else
				case ImageIo::CM_RGB:
					mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA : GL_RGB;
					break;
				case ImageIo::CM_GRAY:
					mObj->mInternalFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
					break;
#endif
				default:
					throw ImageIoExceptionIllegalColorModel();
					break;
			}
		}
		else {
			mObj->mInternalFormat = format.getInternalFormat();
		}

		// setup an appropriate dataFormat/ImageTargetTexture based on the image's color space
		GLint dataFormat;
		ImageIo::ChannelOrder channelOrder;
		bool isGray = false;
		switch( imageSources[ i ]->getColorModel() ) {
			case ImageSource::CM_RGB:
				dataFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA : GL_RGB;
				channelOrder = ( imageSources[ i ]->hasAlpha() ) ? ImageIo::RGBA : ImageIo::RGB;
				break;
			case ImageSource::CM_GRAY:
				dataFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
				channelOrder = ( imageSources[ i ]->hasAlpha() ) ? ImageIo::YA : ImageIo::Y;
				isGray = true;
				break;
			default: // if this is some other color space, we'll have to punt and go w/ RGB
				dataFormat = ( imageSources[ i ]->hasAlpha() ) ? GL_RGBA : GL_RGB;
				channelOrder = ( imageSources[ i ]->hasAlpha() ) ? ImageIo::RGBA : ImageIo::RGB;
				break;
		}

		if( imageSources[ i ]->getDataType() == ImageIo::UINT8 ) {
			shared_ptr<ImageTargetGLCubeMap<uint8_t> > target = ImageTargetGLCubeMap<uint8_t>::createRef( this, channelOrder, isGray, imageSources[ i ]->hasAlpha() );
			imageSources[ i ]->load( target );
			glTexImage2D( targets[ i ], 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_UNSIGNED_BYTE, target->getData() );
		}
		else if( imageSources[ i ]->getDataType() == ImageIo::UINT16 ) {
			shared_ptr<ImageTargetGLCubeMap<uint16_t> > target = ImageTargetGLCubeMap<uint16_t>::createRef( this, channelOrder, isGray, imageSources[ i ]->hasAlpha() );
			imageSources[ i ]->load( target );
			glTexImage2D( targets[ i ], 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_UNSIGNED_SHORT, target->getData() );
		}
		else {
			shared_ptr<ImageTargetGLCubeMap<float> > target = ImageTargetGLCubeMap<float>::createRef( this, channelOrder, isGray, imageSources[ i ]->hasAlpha() );
			imageSources[ i ]->load( target );
			glTexImage2D( targets[ i ], 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_FLOAT, target->getData() );
		}
	}
}

GLint CubeMap::getWidth() const
{
	return mObj->mWidth;
}

GLint CubeMap::getHeight() const
{
	return mObj->mHeight;
}

void CubeMap::enableAndBind() const
{
	glEnable( GL_TEXTURE_CUBE_MAP );
	glBindTexture( mObj->mTarget, mObj->mTextureID );
}

void CubeMap::bind( GLuint textureUnit ) const
{
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( mObj->mTarget, mObj->mTextureID );
	glActiveTexture( GL_TEXTURE0 );
}

void CubeMap::unbind( GLuint textureUnit ) const
{
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glBindTexture( mObj->mTarget, 0 );
	glActiveTexture( GL_TEXTURE0 );
}

void CubeMap::disable() const
{
	glDisable( GL_TEXTURE_CUBE_MAP );
}

CubeMap::Obj::~Obj()
{
	if( ( mTextureID > 0 ) ) {
		glDeleteTextures( 1, &mTextureID );
	}
}

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetGLTexture
template<typename T>
shared_ptr<ImageTargetGLCubeMap<T> > ImageTargetGLCubeMap<T>::createRef( const CubeMap *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha )
{
	return shared_ptr<ImageTargetGLCubeMap<T> >( new ImageTargetGLCubeMap<T>( aTexture, aChannelOrder, aIsGray, aHasAlpha ) );
}

template<typename T>
ImageTargetGLCubeMap<T>::ImageTargetGLCubeMap( const CubeMap *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha )
    : ImageTarget(), mTexture( aTexture ), mIsGray( aIsGray ), mHasAlpha( aHasAlpha )
{
	if( mIsGray )
		mPixelInc = ( mHasAlpha ) ? 2 : 1;
	else
		mPixelInc = ( mHasAlpha ) ? 4 : 3;
	mRowInc = mTexture->getWidth() * mPixelInc;
	// allocate enough room to hold all these pixels
	mData = new T[mTexture->getHeight() * mRowInc];

	if( boost::is_same<T,uint8_t>::value )
		setDataType( ImageIo::UINT8 );
	else if( boost::is_same<T,uint16_t>::value )
		setDataType( ImageIo::UINT16 );
	else if( boost::is_same<T,float>::value )
		setDataType( ImageIo::FLOAT32 );

	setChannelOrder( aChannelOrder );
	setColorModel( mIsGray ? ImageIo::CM_GRAY : ImageIo::CM_RGB );
}

template<typename T>
ImageTargetGLCubeMap<T>::~ImageTargetGLCubeMap()
{
	delete [] mData;
}


void enableReflectionMapping()
{
	glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
	glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
	glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
	glEnable( GL_TEXTURE_GEN_S );
	glEnable( GL_TEXTURE_GEN_T );
	glEnable( GL_TEXTURE_GEN_R );
	glEnable( GL_TEXTURE_CUBE_MAP );
}

void disableReflectionMapping()
{
	glDisable( GL_TEXTURE_GEN_S );
	glDisable( GL_TEXTURE_GEN_T );
	glDisable( GL_TEXTURE_GEN_R );
	glDisable( GL_TEXTURE_CUBE_MAP );
}

} } // namespace mndl::gl

