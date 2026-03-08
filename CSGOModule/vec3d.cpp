#include "DllModule.h"
#include <iomanip>
#include <random>
#include <DirectXMath.h>

vec3_t::vec3_t(void) {
	x = y = z = 0.0f;
}

vec3_t::vec3_t(float _x, float _y, float _z) {
	x = _x;
	y = _y;
	z = _z;
}

vec3_t::~vec3_t(void) {};

void vec3_t::init(float _x, float _y, float _z) {
	x = _x; y = _y; z = _z;
}

void vec3_t::clamp(void) {
	x = std::clamp(x, -89.0f, 89.0f);
	y = std::clamp(std::remainder(y, 360.0f), -180.0f, 180.0f);
	z = std::clamp(z, -50.0f, 50.0f);
}

void  vec3_t::Set(float x = 0.0f, float y = 0.0f, float z = 0.0f)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

void vec3_t::clear(void)
{
	this->Set(0, 0, 0);
}

vec3_t vec3_t::clamped() {
	vec3_t clamped = *this;
	clamped.clamp();
	return clamped;
}

float vec3_t::distance_to(const vec3_t& other) {
	vec3_t delta;
	delta.x = x - other.x;
	delta.y = y - other.y;
	delta.z = z - other.z;

	return delta.length();
}

void vec3_t::normalize() {
	x = std::isfinite(x) ? std::remainderf(x, 360.0f) : 0.0f;
	y = std::isfinite(y) ? std::remainderf(y, 360.0f) : 0.0f;
	//z = 0.0f;
}

float vec3_t::NormalizeInPlace()
{
	vec3_t& v = *this;

	float iradius = 1.0f / (this->Length() + FLT_EPSILON);

	v.x *= iradius;
	v.y *= iradius;
	v.z *= iradius;
	return iradius;
}

float vec3_t::Normalize() const
{
	vec3_t res = *this;
	float l = res.Length();

	if (l)
		res /= l;
	else
		res.x = res.y = res.z = 0.0f;

	return l;
}

vec3_t vec3_t::normalized(void) {
	vec3_t vec(*this);
	vec.normalize();

	return vec;
}

vec3_t vec3_t::normalize_2() const
{
	vec3_t vec(*this);

	float radius = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);

	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	float iradius = 1.f / (radius + FLT_EPSILON);

	vec.x *= iradius;
	vec.y *= iradius;
	vec.z *= iradius;

	return vec;
}

float  vec3_t::weird_normalize()
{
	/*auto length = this->Length();

	(*this) /= length;

	return length;*/
	float len = Length();

	(*this) /= (len + std::numeric_limits< float >::epsilon());

	return len;
}

vec3_t vec3_t::Normalized() const
{
	vec3_t res = *this;
	float l = res.Length();

	if (l)
		res /= l;
	else
		res.x = res.y = res.z = 0.0f;

	return res;
}
/*
vec3_t vec3_t::Normalized() const
{
	vec3_t vec(*this);

	float radius = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);

	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	float iradius = 1.f / (radius + FLT_EPSILON);

	vec.x *= iradius;
	vec.y *= iradius;
	vec.z *= iradius;

	return vec;
}*/

float vec3_t::Length() const
{
	return sqrt(x * x + y * y + z * z);
}

float vec3_t::length(void) {
	float root = 0.0f, sqsr = this->length_sqr();

	__asm        sqrtss xmm0, sqsr
	__asm        movss root, xmm0

	return root;
}

float vec3_t::length_sqr(void) {
	auto sqr = [](float n) {
		return static_cast<float>(n * n);
	};

	return (sqr(x) + sqr(y) + sqr(z));
}

float vec3_t::length_2d_sqr(void) const {
	return (x * x + y * y);
}

float vec3_t::dot(const vec3_t other) {
	return (x * other.x + y * other.y + z * other.z);
}

float vec3_t::dot(const float* other) {
	const vec3_t& a = *this;

	return(a.x * other[0] + a.y * other[1] + a.z * other[2]);
}

void matrix_t::MatrixSetColumn(const vec3_t& in, int column)
{
	mat_val[0][column] = in.x;
	mat_val[1][column] = in.y;
	mat_val[2][column] = in.z;
}

void matrix_t::AngleMatrix(const vec3_t& angles, const vec3_t& position)
{
	AngleMatrix(angles);
	MatrixSetColumn(position, 3);
}

void matrix_t::AngleMatrix(const vec3_t& angles)
{
	float sr, sp, sy, cr, cp, cy;
	DirectX::XMScalarSinCos(&sy, &cy, ToRadians(angles[1]));
	DirectX::XMScalarSinCos(&sp, &cp, ToRadians(angles[0]));
	DirectX::XMScalarSinCos(&sr, &cr, ToRadians(angles[2]));

	mat_val[0][0] = cp * cy;
	mat_val[1][0] = cp * sy;
	mat_val[2][0] = -sp;

	float crcy = cr * cy;
	float crsy = cr * sy;
	float srcy = sr * cy;
	float srsy = sr * sy;
	mat_val[0][1] = sp * srcy - crsy;
	mat_val[1][1] = sp * srsy + crcy;
	mat_val[2][1] = sr * cp;

	mat_val[0][2] = (sp * crcy + srsy);
	mat_val[1][2] = (sp * crsy - srcy);
	mat_val[2][2] = cr * cp;

	mat_val[0][3] = 0.0f;
	mat_val[1][3] = 0.0f;
	mat_val[2][3] = 0.0f;
}


matrix_t matrix_t::ConcatTransforms(matrix_t in) const {
	auto& m = mat_val;
	matrix_t out;
	out[0][0] = m[0][0] * in[0][0] + m[0][1] * in[1][0] + m[0][2] * in[2][0];
	out[0][1] = m[0][0] * in[0][1] + m[0][1] * in[1][1] + m[0][2] * in[2][1];
	out[0][2] = m[0][0] * in[0][2] + m[0][1] * in[1][2] + m[0][2] * in[2][2];
	out[0][3] = m[0][0] * in[0][3] + m[0][1] * in[1][3] + m[0][2] * in[2][3] + m[0][3];
	out[1][0] = m[1][0] * in[0][0] + m[1][1] * in[1][0] + m[1][2] * in[2][0];
	out[1][1] = m[1][0] * in[0][1] + m[1][1] * in[1][1] + m[1][2] * in[2][1];
	out[1][2] = m[1][0] * in[0][2] + m[1][1] * in[1][2] + m[1][2] * in[2][2];
	out[1][3] = m[1][0] * in[0][3] + m[1][1] * in[1][3] + m[1][2] * in[2][3] + m[1][3];
	out[2][0] = m[2][0] * in[0][0] + m[2][1] * in[1][0] + m[2][2] * in[2][0];
	out[2][1] = m[2][0] * in[0][1] + m[2][1] * in[1][1] + m[2][2] * in[2][1];
	out[2][2] = m[2][0] * in[0][2] + m[2][1] * in[1][2] + m[2][2] * in[2][2];
	out[2][3] = m[2][0] * in[0][3] + m[2][1] * in[1][3] + m[2][2] * in[2][3] + m[2][3];
	return out;
}

vec3_t matrix_t::operator*(const vec3_t& vVec) const {
	auto& m = mat_val;
	vec3_t vRet;
	vRet.x = m[0][0] * vVec.x + m[0][1] * vVec.y + m[0][2] * vVec.z + m[0][3];
	vRet.y = m[1][0] * vVec.x + m[1][1] * vVec.y + m[1][2] * vVec.z + m[1][3];
	vRet.z = m[2][0] * vVec.x + m[2][1] * vVec.y + m[2][2] * vVec.z + m[2][3];

	return vRet;
}

matrix_t matrix_t::operator+(const matrix_t& other) const {
	matrix_t ret;
	auto& m = mat_val;
	for (int i = 0; i < 12; i++) {
		((float*)ret.mat_val)[i] = ((float*)m)[i] + ((float*)other.mat_val)[i];
	}
	return ret;
}

matrix_t matrix_t::operator-(const matrix_t& other) const {
	matrix_t ret;
	auto& m = mat_val;
	for (int i = 0; i < 12; i++) {
		((float*)ret.mat_val)[i] = ((float*)m)[i] - ((float*)other.mat_val)[i];
	}
	return ret;
}

matrix_t matrix_t::operator*(const float& other) const {
	matrix_t ret;
	auto& m = mat_val;
	for (int i = 0; i < 12; i++) {
		((float*)ret.mat_val)[i] = ((float*)m)[i] * other;
	}
	return ret;
}
