#pragma once
#pragma once
#pragma warning( disable : 26451 )
#include <limits>
#include <algorithm>

inline float bits_to_float(std::uint32_t i) {
	union convertor_t {
		float f; unsigned long ul;
	} tmp;

	tmp.ul = i;
	return tmp.f;
}

constexpr double M_PI = 3.14159265358979323846;
constexpr float M_RADPI = 57.295779513082f;
constexpr float M_PI_F = static_cast<float>(M_PI);

constexpr float RAD2DEG(const float x) {
	return (float)(x) * (float)(180.f / M_PI_F);
}
constexpr float DEG2RAD(const float x) {
	return (float)(x) * (float)(M_PI_F / 180.f);
}

constexpr std::uint32_t FLOAT32_NAN_BITS = 0x7FC00000;
const float FLOAT32_NAN = bits_to_float(FLOAT32_NAN_BITS);
#define VEC_T_NAN FLOAT32_NAN
#define ASSERT( _exp ) ( (void ) 0 )

class vec3_t {
public:
	vec3_t();
	vec3_t(float, float, float);
	~vec3_t();

	float x, y, z;
	bool operator!=(const vec3_t& v) {
		return (v.x != x) || (v.y != y) || (v.z != z);
	}
	bool operator==(vec3_t& v) {
		return (v.x == x) && (v.y == y) && (v.z == z);
	}
	vec3_t& operator+=(const vec3_t& v) {
		x += v.x; y += v.y; z += v.z; return *this;
	}
	vec3_t& operator-=(const vec3_t& v) {
		x -= v.x; y -= v.y; z -= v.z; return *this;
	}
	vec3_t& operator*=(float v) {
		x *= v; y *= v; z *= v; return *this;
	}
	vec3_t operator+(const vec3_t& v) {
		return vec3_t{ x + v.x, y + v.y, z + v.z };
	}
	vec3_t operator+(float v) const {
		return vec3_t(x + v, y + v, z + v);
	}
	vec3_t operator-(const vec3_t& v) {
		return vec3_t{ x - v.x, y - v.y, z - v.z };
	}
	vec3_t operator*(float fl) const {
		return vec3_t(x * fl, y * fl, z * fl);
	}
	vec3_t operator*(const vec3_t& v) const {
		return vec3_t(x * v.x, y * v.y, z * v.z);
	}
	vec3_t& operator/=(float fl) {
		x /= fl;
		y /= fl;
		z /= fl;
		return *this;
	}

	vec3_t operator+(const vec3_t& v) const
	{
		return vec3_t(x + v.x, y + v.y, z + v.z);
	}

	auto operator-(const vec3_t& other) const -> vec3_t {
		auto buf = *this;

		buf.x -= other.x;
		buf.y -= other.y;
		buf.z -= other.z;

		return buf;
	}

	auto operator/(float other) const {
		vec3_t vec;
		vec.x = x / other;
		vec.y = y / other;
		vec.z = z / other;
		return vec;
	}

	float& operator[](int i) {
		return ((float*)this)[i];
	}
	float operator[](int i) const {
		return ((float*)this)[i];
	}

	inline float length_2d() const {
		return sqrt((x * x) + (y * y));
	}
	void crossproduct(vec3_t v1, vec3_t v2, vec3_t cross_p) const {
		cross_p.x = (v1.y * v2.z) - (v1.z * v2.y); //i
		cross_p.y = -((v1.x * v2.z) - (v1.z * v2.x)); //j
		cross_p.z = (v1.x * v2.y) - (v1.y * v2.x); //k
	}

	inline bool valid()
	{
		return *this != vec3_t(0.f, 0.f, 0.f);
	}

	void VectorCrossProduct(const vec3_t& a, const vec3_t& b, vec3_t& result)
	{
		result.x = a.y * b.z - a.z * b.y;
		result.y = a.z * b.x - a.x * b.z;
		result.z = a.x * b.y - a.y * b.x;
	}

	vec3_t Cross(const vec3_t& vOther)
	{
		vec3_t res;
		VectorCrossProduct(*this, vOther, res);

		return res;
	}

	vec3_t cross(const vec3_t& other) const {
		vec3_t res;
		crossproduct(*this, other, res);
		return res;
	}
	bool is_zero() const {
		return x == 0 && y == 0 && z == 0;
	}

	__forceinline float dot(const vec3_t& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	__forceinline float dot(const float* other) const
	{
		return x * other[0] + y * other[1] + z * other[2];
	}

	auto normalize_angle()
	{
		auto x_rev = this->x / 360.f;
		if (this->x > 180.f || this->x < -180.f)
		{
			x_rev = abs(x_rev);
			x_rev = round(x_rev);

			if (this->x < 0.f)
				this->x = (this->x + 360.f * x_rev);

			else
				this->x = (this->x - 360.f * x_rev);
		}

		auto y_rev = this->y / 360.f;
		if (this->y > 180.f || this->y < -180.f)
		{
			y_rev = abs(y_rev);
			y_rev = round(y_rev);

			if (this->y < 0.f)
				this->y = (this->y + 360.f * y_rev);

			else
				this->y = (this->y - 360.f * y_rev);
		}

		auto z_rev = this->z / 360.f;
		if (this->z > 180.f || this->z < -180.f)
		{
			z_rev = abs(z_rev);
			z_rev = round(z_rev);

			if (this->z < 0.f)
				this->z = (this->z + 360.f * z_rev);

			else
				this->z = (this->z - 360.f * z_rev);
		}
	}

	void to_vectors(vec3_t& right, vec3_t& up) {
		vec3_t tmp;
		if (x == 0.f && y == 0.f)
		{
			right[0] = 0.f;
			right[1] = -1.f;
			right[2] = 0.f;
			up[0] = -z;
			up[1] = 0.f;
			up[2] = 0.f;
		}
		else
		{
			tmp[0] = 0; tmp[1] = 0; tmp[2] = 1;

			// get directions vector using cross product.
			right = Cross(tmp).Normalized();
			up = right.Cross(*this).Normalized();
		}
	}

	void init(float ix, float iy, float iz);
	void clamp();
	void Set(float x, float y, float z);
	void clear();
	vec3_t clamped();
	vec3_t normalized();
	vec3_t normalize_2() const;
	float weird_normalize();
	vec3_t Normalized() const;
	float Length() const;
	float distance_to(const vec3_t& other);
	void normalize();
	float NormalizeInPlace();
	float Normalize() const;
	float length();
	float length_sqr();
	float length_2d_sqr(void) const;
	float dot(const vec3_t other);
	float dot(const float* other);
};

inline vec3_t operator*(float lhs, const vec3_t& rhs) {
	return vec3_t(rhs.x * lhs, rhs.x * lhs, rhs.x * lhs);
}

constexpr auto RadPi = 3.14159265358979323846;
constexpr auto DegPi = 180.0;

class quaternion
{
public:
	float x, y, z, w;
};

struct matrix_t {
	matrix_t() { }
	matrix_t(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23)
	{
		mat_val[0][0] = m00;	mat_val[0][1] = m01; mat_val[0][2] = m02; mat_val[0][3] = m03;
		mat_val[1][0] = m10;	mat_val[1][1] = m11; mat_val[1][2] = m12; mat_val[1][3] = m13;
		mat_val[2][0] = m20;	mat_val[2][1] = m21; mat_val[2][2] = m22; mat_val[2][3] = m23;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	void init(const vec3_t& x, const vec3_t& y, const vec3_t& z, const vec3_t& origin) {
		mat_val[0][0] = x.x; mat_val[0][1] = y.x; mat_val[0][2] = z.x; mat_val[0][3] = origin.x;
		mat_val[1][0] = x.y; mat_val[1][1] = y.y; mat_val[1][2] = z.y; mat_val[1][3] = origin.y;
		mat_val[2][0] = x.z; mat_val[2][1] = y.z; mat_val[2][2] = z.z; mat_val[2][3] = origin.z;
	}

	template<typename T>
	T ToRadians(T degrees) {
		return (degrees * (static_cast<T>(RadPi) / static_cast<T>(DegPi)));
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	matrix_t(const vec3_t& x, const vec3_t& y, const vec3_t& z, const vec3_t& origin) {
		init(x, y, z, origin);
	}

	inline void set_origin(vec3_t const& p) {
		mat_val[0][3] = p.x;
		mat_val[1][3] = p.y;
		mat_val[2][3] = p.z;
	}

	inline vec3_t get_origin()
	{
		vec3_t vecRet(mat_val[0][3], mat_val[1][3], mat_val[2][3]);
		return vecRet;
	}

	inline void invalidate(void) {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				mat_val[i][j] = VEC_T_NAN;
			}
		}
	}

	void QuaternionMatrix(const quaternion& q)
	{
		mat_val[0][0] = 1.0 - 2.0 * q.y * q.y - 2.0 * q.z * q.z;
		mat_val[1][0] = 2.0 * q.x * q.y + 2.0 * q.w * q.z;
		mat_val[2][0] = 2.0 * q.x * q.z - 2.0 * q.w * q.y;

		mat_val[0][1] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
		mat_val[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
		mat_val[2][1] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;

		mat_val[0][2] = 2.0f * q.x * q.z + 2.0f * q.w * q.y;
		mat_val[1][2] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
		mat_val[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;

		mat_val[0][3] = 0.0f;
		mat_val[1][3] = 0.0f;
		mat_val[2][3] = 0.0f;
	}

	void QuaternionMatrix(const quaternion& q, const vec3_t& pos)
	{
		QuaternionMatrix(q);

		mat_val[0][3] = pos.x;
		mat_val[1][3] = pos.y;
		mat_val[2][3] = pos.z;
	}

	void MatrixSetColumn(const vec3_t& in, int column);

	void AngleMatrix(const vec3_t& angles, const vec3_t& position);

	void AngleMatrix(const vec3_t& angles);

	matrix_t ConcatTransforms(matrix_t in) const;
	vec3_t operator*(const vec3_t& vVec) const;
	matrix_t operator+(const matrix_t& other) const;
	matrix_t operator-(const matrix_t& other) const;
	matrix_t operator*(const float& other) const;
	matrix_t operator*(const matrix_t& vm) const {
		return ConcatTransforms(vm);
	}

	float* operator[](int i) { ASSERT((i >= 0) && (i < 3)); return mat_val[i]; }
	const float* operator[](int i) const { ASSERT((i >= 0) && (i < 3)); return mat_val[i]; }
	float* base() { return &mat_val[0][0]; }
	const float* base() const { return &mat_val[0][0]; }

	float mat_val[3][4];
};
