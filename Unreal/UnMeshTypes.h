#ifndef __UNMESHTYPES_H__
#define __UNMESHTYPES_H__

/*-----------------------------------------------------------------------------
	4-byte packed normal vector
-----------------------------------------------------------------------------*/

// used in Bioshock and in UE3
struct FPackedNormal
{
	unsigned				Data;

	friend FArchive& operator<<(FArchive &Ar, FPackedNormal &N)
	{
		return Ar << N.Data;
	}

	operator FVector() const
	{
		// "x / 127.5 - 1" comes from Common.usf, TangentBias() macro
		FVector r;
		r.X = ( Data        & 0xFF) / 127.5f - 1;
		r.Y = ((Data >> 8 ) & 0xFF) / 127.5f - 1;
		r.Z = ((Data >> 16) & 0xFF) / 127.5f - 1;
		return r;
	}

	FPackedNormal &operator=(const FVector &V)
	{
		Data = int((V.X + 1) * 255)
			+ (int((V.Y + 1) * 255) << 8)
			+ (int((V.Z + 1) * 255) << 16);
		return *this;
	}

	float GetW() const
	{
		return (Data >> 24) / 127.5 - 1;
	}
};


/*-----------------------------------------------------------------------------
	Different compressed quaternion types
-----------------------------------------------------------------------------*/

// really have data, when W*W == -0.0f, and sqrt(wSq) was returned -INF
#define RESTORE_QUAT_W(r)									\
		float wSq = 1.0f - (r.X*r.X + r.Y*r.Y + r.Z*r.Z);	\
		r.W = (wSq > 0) ? sqrt(wSq) : 0;


struct FQuatFloat96NoW
{
	float			X, Y, Z;

	inline operator FQuat() const
	{
		FQuat r;
		r.X = X;
		r.Y = Y;
		r.Z = Z;
		RESTORE_QUAT_W(r);
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatFloat96NoW &Q)
	{
		return Ar << Q.X << Q.Y << Q.Z;
	}
};

SIMPLE_TYPE(FQuatFloat96NoW, float)


// normalized quaternion with 3 16-bit fixed point fields
struct FQuatFixed48NoW
{
	word			X, Y, Z;				// unsigned short, corresponds to (float+1)*32767

	inline operator FQuat() const
	{
		FQuat r;
		r.X = (X - 32767) / 32767.0f;
		r.Y = (Y - 32767) / 32767.0f;
		r.Z = (Z - 32767) / 32767.0f;
		RESTORE_QUAT_W(r);
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatFixed48NoW &Q)
	{
		return Ar << Q.X << Q.Y << Q.Z;
	}
};

SIMPLE_TYPE(FQuatFixed48NoW, word)


struct FVectorFixed48
{
	word			X, Y, Z;

	inline operator FVector() const
	{
		FVector r;
		static const float scale = 128.0f / 32767.0f;
		r.X = (X - 32767) * scale;
		r.Y = (Y - 32767) * scale;
		r.Z = (Z - 32767) * scale;
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FVectorFixed48 &V)
	{
		return Ar << V.X << V.Y << V.Z;
	}
};

SIMPLE_TYPE(FVectorFixed48, word)


// normalized quaternion with 11/11/10-bit fixed point fields
struct FQuatFixed32NoW
{
	unsigned		Z:10, Y:11, X:11;

	inline operator FQuat() const
	{
		FQuat r;
		r.X = X / 1023.0f - 1.0f;
		r.Y = Y / 1023.0f - 1.0f;
		r.Z = Z / 511.0f  - 1.0f;
		RESTORE_QUAT_W(r);
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatFixed32NoW &Q)
	{
		return Ar << GET_DWORD(Q);
	}
};

SIMPLE_TYPE(FQuatFixed32NoW, unsigned)


struct FQuatIntervalFixed32NoW
{
	unsigned		Z:10, Y:11, X:11;

	FQuat ToQuat(const FVector &Mins, const FVector &Ranges) const
	{
		FQuat r;
		r.X = (X / 1023.0f - 1.0f) * Ranges.X + Mins.X;
		r.Y = (Y / 1023.0f - 1.0f) * Ranges.Y + Mins.Y;
		r.Z = (Z / 511.0f  - 1.0f) * Ranges.Z + Mins.Z;
		RESTORE_QUAT_W(r);
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatIntervalFixed32NoW &Q)
	{
		return Ar << GET_DWORD(Q);
	}
};

SIMPLE_TYPE(FQuatIntervalFixed32NoW, unsigned)


struct FVectorIntervalFixed32
{
	unsigned		X:10, Y:11, Z:11;

	FVector ToVector(const FVector &Mins, const FVector &Ranges) const
	{
		FVector r;
		r.X = (X / 511.0f  - 1.0f) * Ranges.X + Mins.X;
		r.Y = (Y / 1023.0f - 1.0f) * Ranges.Y + Mins.Y;
		r.Z = (Z / 1023.0f - 1.0f) * Ranges.Z + Mins.Z;
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FVectorIntervalFixed32 &V)
	{
		return Ar << GET_DWORD(V);
	}
};

SIMPLE_TYPE(FVectorIntervalFixed32, unsigned)


struct FQuatFloat32NoW
{
	unsigned		data;

	inline operator FQuat() const
	{
		FQuat r;
		r.X = r.Y = r.Z = 0;

		int _X = data >> 21;			// 11 bits
		int _Y = (data >> 10) & 0x7FF;	// 11 bits
		int _Z = data & 0x3FF;			// 10 bits

		*(unsigned*)&r.X = ((((_X >> 7) & 7) + 123) << 23) | ((_X & 0x7F | 32 * (_X & 0xFFFFFC00)) << 16);
		*(unsigned*)&r.Y = ((((_Y >> 7) & 7) + 123) << 23) | ((_Y & 0x7F | 32 * (_Y & 0xFFFFFC00)) << 16);
		*(unsigned*)&r.Z = ((((_Z >> 6) & 7) + 123) << 23) | ((_Z & 0x3F | 32 * (_Z & 0xFFFFFE00)) << 17);

		RESTORE_QUAT_W(r);
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatFloat32NoW &Q)
	{
		return Ar << Q.data;
	}
};


#if BATMAN

// This is a variant of FQuatFixed48NoW developer for Batman: Arkham Asylum. It's destination
// is to store quaternion with a better precision than FQuatFixed48NoW, but there was a logical
// error: FQuatFixed48NoW's precision is 1/32768, but FQuatFixed48Max's precision is only 1/23170 !
struct FQuatFixed48Max
{
	word			data[3];				// layout: V2[15] : V1[15] : V0[15] : S[2]

	inline operator FQuat() const
	{
		unsigned tmp;
		tmp = (data[1] << 16) | data[0];
		int S = tmp & 3;								// bits [0..1]
		int L = (tmp >> 2) & 0x7FFF;					// bits [2..16]
		tmp = (data[2] << 16) | data[1];
		int M = (tmp >> 1) & 0x7FFF;					// bits [17..31]
		int H = (tmp >> 16) & 0x7FFF;					// bits [32..46]
		// bit 47 is unused ...

		static const float shift = 0.70710678118f;		// sqrt(0.5)
		static const float scale = 1.41421356237f;		// sqrt(0.5)*2
		float l = (L - 0.5f) / 32767 * scale - shift;
		float m = (M - 0.5f) / 32767 * scale - shift;
		float h = (H - 0.5f) / 32767 * scale - shift;
		float a = sqrt(1.0f - (l*l + m*m + h*h));
		// "l", "m", "h" are serialized values, in a range -shift .. +shift
		// if we will remove one of maximal values ("a"), other values will be smaller
		// that "a"; if "a" is 1, all other values are 0; when "a" becomes smaller,
		// other values may grow; maximal value of "other" equals to "a" when
		// 2 quaternion components equals to "a" and 2 other is 0; so, maximal
		// stored value can be computed from equation "a*a + a*a + 0 + 0 = 1", so
		// maximal value is sqrt(1/2) ...
		// "a" is computed value, up to 1

		FQuat r;
		switch (S)			// choose where to place "a"
		{
		case 0:
			r.Set(a, l, m, h);
			break;
		case 1:
			r.Set(l, a, m, h);
			break;
		case 2:
			r.Set(l, m, a, h);
			break;
		default: // 3
			r.Set(l, m, h, a);
		}
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatFixed48Max &Q)
	{
		return Ar << Q.data[0] << Q.data[1] << Q.data[2];
	}
};

SIMPLE_TYPE(FQuatFixed48Max, word)

#endif // BATMAN


#if MASSEFF

// similar to FQuatFixed48Max
struct FQuatBioFixed48
{
	word			data[3];

	inline operator FQuat() const
	{
		FQuat r;
		static const float shift = 0.70710678118f;		// sqrt(0.5)
		static const float scale = 1.41421356237f;		// sqrt(0.5)*2
		float x = (data[0] & 0x7FFF) / 32767.0f * scale - shift;
		float y = (data[1] & 0x7FFF) / 32767.0f * scale - shift;
		float z = (data[2] & 0x7FFF) / 32767.0f * scale - shift;
		float w = 1.0f - (x*x + y*y + z*z);
		if (w >= 0.0f)
			w = sqrt(w);
		else
			w = 0.0f;
		int s = ((data[0] >> 14) & 2) | ((data[1] >> 15) & 1);
		switch (s)
		{
		case 0:
			r.Set(w, x, y, z);
			break;
		case 1:
			r.Set(x, w, y, z);
			break;
		case 2:
			r.Set(x, y, w, z);
			break;
		default: // 3
			r.Set(x, y, z, w);
		}
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatBioFixed48 &Q)
	{
		return Ar << Q.data[0] << Q.data[1] << Q.data[2];
	}
};

SIMPLE_TYPE(FQuatBioFixed48, word)

#endif // MASSEFF


#if TRANSFORMERS

// mix of FQuatFixed48NoW and FQuatIntervalFixed32NoW
struct FQuatIntervalFixed48NoW
{
	word			X, Y, Z;

	FQuat ToQuat(const FVector &Mins, const FVector &Ranges) const
	{
		FQuat r;
		r.X = (X - 32767) / 32767.0f * Ranges.X + Mins.X;
		r.Y = (Y - 32767) / 32767.0f * Ranges.Y + Mins.Y;
		r.Z = (Z - 32767) / 32767.0f * Ranges.Z + Mins.Z;
		RESTORE_QUAT_W(r);
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatIntervalFixed48NoW &Q)
	{
		return Ar << Q.X << Q.Y << Q.Z;
	}
};

SIMPLE_TYPE(FQuatIntervalFixed48NoW, word)


struct FPackedVectorTrans
{
	unsigned		Z:11, Y:10, X:11;

	FVector ToVector(const FVector &Mins, const FVector &Ranges) const
	{
		FVector r;
		r.X = X / 2047.0f * Ranges.X + Mins.X;
		r.Y = Y / 1023.0f * Ranges.Y + Mins.Y;
		r.Z = Z / 2047.0f * Ranges.Z + Mins.Z;
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FPackedVectorTrans &Q)
	{
		return Ar << GET_DWORD(Q);
	}
};

SIMPLE_TYPE(FPackedVectorTrans, unsigned)

#endif // TRANSFORMERS


#if BORDERLANDS

struct FQuatDelta48NoW
{
	word			X, Y, Z;

	FQuat ToQuat(const FVector &Mins, const FVector &Ranges, const FQuat &Base) const
	{
		FQuat r;
		int Sign = Z >> 15;
		r.X = X / 65535.0f * Ranges.X + Mins.X + Base.X;
		r.Y = Y / 65535.0f * Ranges.Y + Mins.Y + Base.Y;
		r.Z = (Z & 0x7FFF) / 32767.0f * Ranges.Z + Mins.Z + Base.Z;
		RESTORE_QUAT_W(r);
		if (Sign) r.W = -r.W;
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FQuatDelta48NoW &Q)
	{
		return Ar << Q.X << Q.Y << Q.Z;
	}
};

struct FVectorDelta48NoW
{
	word			X, Y, Z;

	FVector ToVector(const FVector &Mins, const FVector &Ranges, const FVector &Base) const
	{
		FVector r;
		r.X = X / 65535.0f * Ranges.X + Mins.X + Base.X;
		r.Y = Y / 65535.0f * Ranges.Y + Mins.Y + Base.Y;
		r.Z = Z / 65535.0f * Ranges.Z + Mins.Z + Base.Z;
		return r;
	}

	friend FArchive& operator<<(FArchive &Ar, FVectorDelta48NoW &V)
	{
		return Ar << V.X << V.Y << V.Z;
	}
};

#endif // BORDERLANDS


#endif // __UNMESHTYPES_H__
