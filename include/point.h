#ifndef  POINT

#define POINT


class point
{
public:
	float x, y, z;
	float nx, ny, nz;
	float u, v;

	point(float X, float Y, float Z, float nX, float nY, float nZ)
	{
		x = X;
		y = Y;
		z = Z;
		nx = nX;
		ny = nY;
		nz = nZ;
		u = 0.f;
		v = 0.f;
	}
	point(float X, float Y, float Z)
	{
		x = X;
		y = Y;
		z = Z;
		nx = 0.f;
		ny = 0.f;
		nz = 0.f;
		u = 0.f;
		v = 0.f;
	}
	point()
	{
		x = 0.f;
		y = 0.f;
		z = 0.f;
		nx = 0.f;
		ny = 0.f;
		nz = 0.f;
		u = 0.f;
		v = 0.f;
	}
	friend point operator*(float lhs, point rhs)
	{
		point p;
		p.x = rhs.x * lhs;
		p.y = rhs.y * lhs;
		p.z = rhs.z * lhs;
		p.nx = rhs.nx * lhs;
		p.ny = rhs.ny * lhs;
		p.nz = rhs.nz * lhs;
		return p;
	}
	friend point operator+(point lhs, point rhs)
	{
		point p;
		p.x = rhs.x + lhs.x;
		p.y = rhs.y + lhs.y;
		p.z = rhs.z + lhs.z;
		p.nx = rhs.nx + lhs.nx;
		p.ny = rhs.ny + lhs.ny;
		p.nz = rhs.nz + lhs.nz;
		return p;
	}

	point copy() {
		point p;
		p.x = x;
		p.y = y;
		p.z = z;
		p.nx = nx;
		p.ny = ny;
		p.nz = nz;
		p.u = u;
		p.v = v;
		return p;
	}

	void reverseNormals() {
		//glm::vec3 norm = glm::inverse(glm::vec3(nx, ny, nz));
		nx *= -1;
		ny *= -1;
		nz *= -1;
	}

	void setNormals(float x, float y, float z) {
		nx = x;
		ny = y;
		nz = z;
	}

};

#endif // ! POINT