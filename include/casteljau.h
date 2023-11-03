#include <list>
#include <vector>
#include <algorithm>

#include "point.h"

point evaluate(float t, std::list<point> P)
{
	std::list<point> Q = P;

	while (Q.size() > 1)
	{
		std::list<point> R;

		for (auto itt = Q.begin(); std::next(itt) != Q.end(); ++itt)
		{
			point a = (point)*itt;
			point b = (point)*std::next(itt);

			point ab = ((1 - t) * a);
			point bc = (t * b);
			point p = ab + bc;
			R.push_back(p);
		}
		Q.clear();
		Q = R;
	}

	point p = Q.front();
	return p;
}

std::vector<point> EvaluateBezierCurve(std::vector<point>ctrl_points, int num_evaluations)
{
	std::list<point> ps(ctrl_points.begin(), ctrl_points.end());
	std::vector<point> curve;

	float offset = (float) 1 / num_evaluations;

	curve.push_back(ctrl_points[0]);

	for (int e = 0; e < num_evaluations; e++)
	{
		point p = evaluate(offset * (e + 1), ps);

		curve.push_back(p);
	}
	float distance = 0.f; 
	for (int i = 0; i < curve.size(); i++)
	{
		if (i == 0) {
			continue;
		}
		point a = curve[i - 1];
		point b = curve[i];

		distance+= glm::distance(glm::vec3(a.x, a.y, a.z), glm::vec3(b.x, b.y, b.z));
	}

	float d = 0.f;
	for (int i = 0; i < curve.size(); i++)
	{
		if (i == 0) {
			curve[i].setNormals(1.f, 0.f, 0.f);
			continue;
		}
		else if (i == curve.size() - 1) {
			curve[i].setNormals(-1.f, 0.f, 0.f);
			continue;
		}
		point a = curve[i - 1];
		point b = curve[i];
		point c = curve[i + 1];

		point tangent = c + (-1 * a);
		glm::vec3 tan = glm::normalize(glm::vec3(tangent.x, tangent.y, tangent.z));

		d += glm::distance(glm::vec3(a.x, a.y, a.z), glm::vec3(b.x, b.y, b.z));
		curve[i].u = d / distance;

		curve[i].setNormals(-tan.y, tan.x, tan.z);
	}

	return curve;
}

void pushPoint(std::vector<float> &verts, point p) {
	verts.push_back(p.x);
	verts.push_back(p.y);
	verts.push_back(p.z);
	verts.push_back(p.u);
	verts.push_back(p.v);
	verts.push_back(p.nx);
	verts.push_back(p.ny);
	verts.push_back(p.nz);
}
float* MakeFloatsFromVector(std::vector<point> curve, int& num_verts, int& num_floats, float zOffset, float width)
{
	std::vector<float> verts;
	int i = 0;

	for (int j = 0; j < curve.size() - 1; j++) {
		point p1 = curve[j];
		point p2 = curve[j + 1];

		point p3 = p1.copy();
		point p4 = p2.copy();
		p3.z += zOffset;
		p4.z += zOffset;

		p3.v = 1.f;
		p4.v = 1.f;

		// Initial curve Planes
		pushPoint(verts, p1);
		pushPoint(verts, p2);
		pushPoint(verts, p3);

		pushPoint(verts, p2);
		pushPoint(verts, p3);
		pushPoint(verts, p4);

		if (width > 0) {
			float maxU = .3f;

			point p5 = p1.copy();
			point p6 = p2.copy();
			point p7 = p3.copy();
			point p8 = p4.copy();

			p5.y += width;
			p6.y += width;
			p7.y += width;
			p8.y += width;

			p5.reverseNormals();
			p6.reverseNormals();
			p7.reverseNormals();
			p8.reverseNormals();

			// Top curve portion
			pushPoint(verts, p5);
			pushPoint(verts, p6);
			pushPoint(verts, p7);

			pushPoint(verts, p6);
			pushPoint(verts, p7);
			pushPoint(verts, p8);

			// Side Connection between upper and lower curves

			p1.u = (p1.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p2.u = (p2.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p3.u = (p3.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p4.u = (p4.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p5.u = (p5.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p6.u = (p6.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p7.u = (p7.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
			p8.u = (p8.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);

			p1.v = (p1.y - curve[0].y) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p2.v = (p2.y - curve[0].y) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p3.v = (p3.y - curve[0].y) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p4.v = (p4.y - curve[0].y) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p5.v = (p5.y - (curve[0].y)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p6.v = (p6.y - (curve[0].y)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p7.v = (p7.y - (curve[0].y)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;
			p8.v = (p8.y - (curve[0].y)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU + .5f;


			p1.setNormals(0.f, 0.f, -1.f);
			p2.setNormals(0.f, 0.f, -1.f);
			p5.setNormals(0.f, 0.f, -1.f);
			p6.setNormals(0.f, 0.f, -1.f);

			pushPoint(verts, p1);
			pushPoint(verts, p5);
			pushPoint(verts, p6);

			pushPoint(verts, p1);
			pushPoint(verts, p2);
			pushPoint(verts, p6);

			p3.setNormals(0.f, 0.f, 1.f);
			p4.setNormals(0.f, 0.f, 1.f);
			p7.setNormals(0.f, 0.f, 1.f);
			p8.setNormals(0.f, 0.f, 1.f);

			pushPoint(verts, p3);
			pushPoint(verts, p7);
			pushPoint(verts, p8);

			pushPoint(verts, p3);
			pushPoint(verts, p4);
			pushPoint(verts, p8);
		}


	}

	// quads covering z direction y width
	if (width > 0) {
		point p1 = curve[0];
		point p3 = p1.copy();
		p3.z += zOffset;
		p3.v = 1.f;

		point p5 = p1.copy();
		point p7 = p3.copy();

		p5.y += width;
		p7.y += width;
		p5.u = .2f;
		p7.u = .2f;


		p1.setNormals(1.f, 0.f, 0.f);
		p3.setNormals(1.f, 0.f, 0.f);
		p5.setNormals(1.f, 0.f, 0.f);
		p7.setNormals(1.f, 0.f, 0.f);

		pushPoint(verts, p1);
		pushPoint(verts, p3);
		pushPoint(verts, p5);

		pushPoint(verts, p3);
		pushPoint(verts, p5);
		pushPoint(verts, p7);
	
		p1 = curve[curve.size() - 1];
		p3 = p1.copy();
		p3.z += zOffset;
		p3.v = 1.f;

		p5 = p1.copy();
		p7 = p3.copy();

		p5.y += width;
		p7.y += width;
		p5.u = .2f;
		p7.u = .2f;

		p1.setNormals(-1.f, 0.f, 0.f);
		p3.setNormals(-1.f, 0.f, 0.f);
		p5.setNormals(-1.f, 0.f, 0.f);
		p7.setNormals(-1.f, 0.f, 0.f);

		pushPoint(verts, p1);
		pushPoint(verts, p3);
		pushPoint(verts, p5);

		pushPoint(verts, p3);
		pushPoint(verts, p5);
		pushPoint(verts, p7);
	}

	float maxU = .5f;
	// Semi circle sides
	for (int i = 0; i < (curve.size() - 1) / 2; i++) {
		point p1 = curve[i];
		point p2 = curve[i+1];
		point p3 = curve[curve.size() -i -1];
		point p4 = curve[curve.size() -i -2];

		p1.y += width;
		p2.y += width;
		p3.y += width;
		p4.y += width;

		p1.setNormals(0.f, 0.f, -1.f);
		p2.setNormals(0.f, 0.f, -1.f);
		p3.setNormals(0.f, 0.f, -1.f);
		p4.setNormals(0.f, 0.f, -1.f);

		p1.u = (p1.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
		p2.u = (p2.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
		p3.u = (p3.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
		p4.u = (p4.x - curve[0].x) / (curve[curve.size() - 1].x - curve[0].x);
		
		p1.v = (p1.y - (curve[0].y + width)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU;
		p2.v = (p2.y - (curve[0].y + width)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU;
		p3.v = (p3.y - (curve[0].y + width)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU;
		p4.v = (p4.y - (curve[0].y + width)) / (curve[(curve.size() - 1) / 2].y - curve[0].y) * maxU;


		pushPoint(verts, p1);
		pushPoint(verts, p2);
		pushPoint(verts, p3);

		pushPoint(verts, p2);
		pushPoint(verts, p3);
		pushPoint(verts, p4);
		
		p1.z += zOffset;
		p2.z += zOffset;
		p3.z += zOffset;
		p4.z += zOffset;

		p1.setNormals(0.f, 0.f, 1.f);
		p2.setNormals(0.f, 0.f, 1.f);
		p3.setNormals(0.f, 0.f, 1.f);
		p4.setNormals(0.f, 0.f, 1.f);

		pushPoint(verts, p1);
		pushPoint(verts, p2);
		pushPoint(verts, p3);

		pushPoint(verts, p2);
		pushPoint(verts, p3);
		pushPoint(verts, p4);
	}

	num_floats = verts.size();
	num_verts = num_floats / 6;
	if (num_verts == 0)
	{
		return NULL;
	}

	float* arr = new float[verts.size()];
	std::copy(verts.begin(), verts.end(), arr);
	return arr;

} 