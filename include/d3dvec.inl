#ifndef __WINE_D3DVEC_INL
#define __WINE_D3DVEC_INL

/*** constructors ***/

inline _D3DVECTOR::_D3DVECTOR(D3DVALUE f)
{
  x = y = z = f;
}

inline _D3DVECTOR::_D3DVECTOR(D3DVALUE _x, D3DVALUE _y, D3DVALUE _z)
{
  x = _x; y = _y; z = _z;
}

/*** assignment operators ***/

inline _D3DVECTOR& _D3DVECTOR::operator += (const _D3DVECTOR& v)
{
  x += v.x; y += v.y; z += v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator -= (const _D3DVECTOR& v)
{
  x -= v.x; y -= v.y; z -= v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator *= (const _D3DVECTOR& v)
{
  x *= v.x; y *= v.y; z *= v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator /= (const _D3DVECTOR& v)
{
  x /= v.x; y /= v.y; z /= v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator *= (D3DVALUE s)
{
  x *= s; y *= s; z *= s;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator /= (D3DVALUE s)
{
  x /= s; y /= s; z /= s;
  return *this;
}

/*** binary operators ***/

inline _D3DVECTOR operator + (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z);
}

inline _D3DVECTOR operator - (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z);
}

inline _D3DVECTOR operator * (const _D3DVECTOR& v, D3DVALUE s)
{
  return _D3DVECTOR(v.x*s, v.y*s, v.z*s);
}

inline _D3DVECTOR operator * (D3DVALUE s, const _D3DVECTOR& v)
{
  return _D3DVECTOR(v.x*s, v.y*s, v.z*s);
}

inline _D3DVECTOR operator / (const _D3DVECTOR& v, D3DVALUE s)
{
  return _D3DVECTOR(v.x/s, v.y/s, v.z/s);
}

inline D3DVALUE SquareMagnitude(const _D3DVECTOR& v)
{
  return v.x*v.x + v.y*v.y + v.z*v.z; /* DotProduct(v, v) */
}

inline D3DVALUE Magnitude(const _D3DVECTOR& v)
{
  return sqrt(SquareMagnitude(v));
}

inline _D3DVECTOR Normalize(const _D3DVECTOR& v)
{
  return v / Magnitude(v);
}

inline D3DVALUE DotProduct(const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

inline _D3DVECTOR CrossProduct(const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  _D3DVECTOR res;
  /* this is a left-handed cross product, right? */
  res.x = v1.y * v2.z - v1.z * v2.y;
  res.y = v1.z * v2.x - v1.x * v2.z;
  res.z = v1.x * v2.y - v1.y * v2.x;
  return res;
}

#endif
