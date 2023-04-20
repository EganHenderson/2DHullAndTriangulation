/* This is an implementation of a 2D triangulation, using the trisection triangulation algorithm and triangulation cleanup algorithm.
* This allows the user to create either random points, mouse drawn points, or a lattice of points (not fully implemented).
* The random set is of 10 points, with incrementation by 10 possible as well. The lattice is NxN, with N starting at 10 as well.
* The triangulation first does a convex hull then finds the centre point.
* After this, the triangulation tries to create all the triangles from points interior to the convex hull.
* Then the triangles are cleaned up using the triangle cleanup algorithm.
* After triangulation, the number of triangles cleaned up, the number of points, and number of triangles are printed to the console.
*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <freeglut.h>
#include <FreeImage.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

//the point structure
typedef struct
{
	int x, y; //x and y coordinates within the window
} point;

//the edge structure
typedef struct
{
	point p1, p2; //the two points making up an edge
} edge;

//the triangle structure
typedef struct
{
	point p1, p2, p3; //the three points making up a triangle
} tri;

//the global structure
typedef struct
{
	int w, h; //w for width and h for height of the window
	int n; //number of points to create
	vector<point> points; //vector of points
	vector<point> coords; //vector for all possible coordinates
	vector<edge> edges; //vector of edges
	vector<tri> tris; //vector of triangles
	bool mouseDraw; //true when drawing points with the mouse
	bool shuffled; //true when the coords vector has been shuffled
} glob;
glob global;

//enums for the menu buttons/options
enum {
	MENU_QUIT, MENU_RANDOM, MENU_TRIANGULATION, MENU_LATTICE, MENU_INCREMENT, MENU_MOUSE
};

//this method creates a set of random points within the bounds of the window
//the point and edge vectors are cleared to ensure the new points are added to an empty vector
void random()
{
	vector<point>().swap(global.points); //clear the points vector, getting rid of its contents and freeing some memory
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	vector<tri>().swap(global.tris); //clear the tris vector, getting rid of its contents and freeing some memory
	random_shuffle(global.coords.begin(), global.coords.end()); //shuffle the coordinate vector

	//we have shuffled, so set the shuffle bool to true if it isn't already
	if (!global.shuffled)
		global.shuffled = true;

	//create 'n' random points from the vector of possible points based on the window size
	for (int i = 0; i < global.n; i++)
		global.points.push_back(point{ global.coords[i].x, global.coords[i].y }); //add the new point to the point vector

	glutPostRedisplay(); //redisplay the window
}

//this method creates a lattice of points of size N by N
void lattice()
{
	vector<point>().swap(global.points); //clear the points vector, getting rid of its contents and freeing some memory
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	vector<tri>().swap(global.tris); //clear the tris vector, getting rid of its contents and freeing some memory

	//create a N by N lattice
	for (int i = 0; i < global.n; i++)
		for (int j = 0; j < global.n; j++)
			global.points.push_back(point{ i * 5, j * 5 }); //add the new point to the point vector

	glutPostRedisplay(); //redisplay the window
}

//create a point where the mouse clicks
//this method ensures that each point is unique and places the points correctly based on the mouse position in the window
//the y value has to be essentially inversed as 0 in the window is bottom left and 0 for the mouse position is top left
void draw_mouse_point(int x, int y)
{
	y = global.h - 10 - y; //subtract the value of y from the max y coordinate

	//ensure the point is unique from all other points
	for (const point& p : global.points)
		if (p.x == x && p.y == y)
			return;

	global.points.push_back(point{ x, y }); //add the point to the global points vector
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	vector<tri>().swap(global.tris); //clear the tris vector, getting rid of its contents and freeing some memory

	glutPostRedisplay(); //redisplay the window
}

//this method is called whenever opengl calls the mouse fun
//it calls the draw_mouse_point when the user is creating points with the mouse and has left clicked somewhere in the window
void mouse(int bin, int state, int x, int y)
{
	if (global.mouseDraw && bin == GLUT_LEFT_BUTTON && state == GLUT_DOWN) draw_mouse_point(x, y);
}

//this method is an implementation of the QuickHull algorithm
//the coefficients to calculated the distance are calculated first
//points for the max and direction vectors are created, and the ints for distance are initialized
//then all points are iterated through, finding the point with the max distance from the line
//if no point is found, that means either all points are interior to the line or there are colinear points
//that means that the edge is added to the global edge vector
//if a point is found, quick_hull is called on the two lines from pMax to the original two points
void quick_hull(point p1, point p2)
{
	//set the coefficients
	int a = p1.y - p2.y;
	int b = p2.x - p1.x;
	int c = p1.x * p2.y - p1.y * p2.x;

	//initalize the points and ints
	point pMax = point{ 0, 0 };
	int maxD = 0;
	int d;
	point dir_p1_p, dir_p_p2;

	//iterate through all points, finding the max distance points
	for (const point& p : global.points)
	{
		if (!(p.x == p1.x && p.y == p1.y || p.x == p2.x && p.y == p2.y))
		{
			d = a * p.x + b * p.y + c;

			if (d > maxD)
			{
				pMax = p;
				maxD = d;
			}
		}
	}

	//if no point is found, add the edge to the vector
	if (maxD == 0)
	{
		global.edges.push_back(edge{ p1, p2 });
		return;
	}

	//recursively call quick_hull
	quick_hull(p1, pMax);
	quick_hull(pMax, p2);
}

//this method creates a convex hull using the quick hull algorithm
//the method finds the point with the minimum x and maximum x values
//then, quick_hull is called for both directions of the line, ensuring we create a top and bottom to the hull
void convex_hull()
{
	//if there are less than three points, we cannot create a convex hull so immediately stop
	if (global.points.size() < 3)
		return;

	//initialize the min and max points
	point minPoint;
	minPoint.x = global.w;
	point maxPoint;
	maxPoint.x = -1;

	//iterate through all points and find the min and max
	for (const point& p : global.points)
	{
		if (p.x < minPoint.x)
			minPoint = p;
		if (p.x > maxPoint.x)
			maxPoint = p;
	}

	//call quick hull for both directions of the line
	quick_hull(minPoint, maxPoint);
	quick_hull(maxPoint, minPoint);
}

//this method checks if a given point exists within the global vector of edges
//returns true when the point is on an edge and false otherwise
bool on_edge(point p)
{
	for (const edge& e : global.edges)
		if ((p.x == e.p1.x && p.y == e.p1.y) || (p.x == e.p2.x && p.y == e.p2.y))
			return true;

	return false;
}

//this method returns the distance between two points using pythagorean theorem
int pythagorean(point p1, point p2)
{
	return sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
}

//this method calculates and returns the distance of point p3 from the line p1p2
int dist(point p1, point p2, point p3)
{
	int d = 0; //initialize the distance int

	d += (p1.y - p2.y) * p3.x; //add the 'A' coefficient to the distance where A = (y1 - y2)
	d += (p2.x - p1.x) * p3.y; //add the 'B' coefficient to the distance where B = (x2 - x1)
	d += p1.x * p2.y - p1.y * p2.x; //add the 'C' coefficient to the distance where C = (x1y2 - y1x2)

	return d;
}

//check if the given point is within the given triangle
//returns true when the point is within, false otherwise
//if the point is colinear, it returns false
bool point_in_triangle(point p, tri t)
{
	//calculate the distances
	int d1 = dist(t.p1, t.p2, p);
	int d2 = dist(t.p2, t.p3, p);

	//if the distances signs don't match, the point is not in the triangle
	if ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0))
		return false;

	//calculate the third distance
	int d3 = dist(t.p3, t.p1, p);

	//if the distances signs don't match, the point is not in the triangle
	if ((d1 > 0 && d3 < 0) || (d1 < 0 && d3 > 0))
		return false;

	//otherwise the point must be within the triangle or is colinear
	return true;
}

//this method trisects a triangle to find if there are any points interior to it to create more triangles
//after finding the extents, all points are iterated through, finding which points are within the triange
//if the point is within the given triangle, then trisect the three triangles formed by that point
void trisect(tri t)
{
	//determine the extents of the triangle by finding the min and max for the x and y coordinates
	int xMax = -1, xMin = global.w, yMax = -1, yMin = global.h;

	xMax = max(t.p1.x, xMax);
	xMax = max(t.p2.x, xMax);
	xMax = max(t.p3.x, xMax);

	yMax = max(t.p1.y, yMax);
	yMax = max(t.p2.y, yMax);
	yMax = max(t.p3.y, yMax);

	xMin = min(t.p1.x, xMin);
	xMin = min(t.p2.x, xMin);
	xMin = min(t.p3.x, xMin);

	yMin = min(t.p1.y, yMin);
	yMin = min(t.p2.y, yMin);
	yMin = min(t.p3.y, yMin);

	bool pFound = false;

	//go through all points, but check if the point is interior to the triangle before doing any trisections
	for (const point& p : global.points)
	{
		//ensure the point is not one of the three making up this triangle
		if (!((p.x == t.p1.x && p.y == t.p1.y) || (p.x == t.p2.x && p.y == t.p2.y) || (p.x == t.p3.x && p.y == t.p3.y)))
		{
			//if the point is within the extents of the triangle, determine if it is interior to the triangle
			if (p.x >= xMin && p.x <= xMax && p.y >= yMin && p.y <= yMax)
			{
				//if the point is within the triangle, call trisect on the three triangles making it up
				//set pFound to true to ensure the encompassing triangle is not added to the global tris vector
				if (point_in_triangle(p, t))
				{
					pFound = true;
					trisect(tri{ t.p1, t.p2, p });
					trisect(tri{ t.p2, t.p3, p });
					trisect(tri{ t.p3, t.p1, p });
				}
			}
		}
	}

	//if we find no interior point, add the triangle to global tri vector
	if (!pFound)
		global.tris.push_back(t);
}

//this method checks the two passed in ints to determine if they have the same sign or not
bool same_sign(int a, int b)
{
	if (a >= 0 && b >= 0)
		return true;
	if (a <= 0 && b <= 0)
		return true;

	return false;
}

//this method checks all possible edges of the two triangles to determine if they share an edge
//if a shared edge is found, we can immediately return as two triangles can only ever share one edge (otherwise one of them wouldn't be a triangle)
//this method sets the two given tris to the calculated tris if a shared edge is found
bool check_shared_edge(tri& t1, tri& t2)
{
	//initialize ints for keeping track of distance and point count, and bools for keeping track of shared points
	int d1, d2, pointCount = 0;
	bool t1p1Share = false, t1p2Share = false, t1p3Share = false;
	bool t2p1Share = false, t2p2Share = false, t2p3Share = false;

	//check if tri 1's p1 is the same as any point in tri 2, then check t1.p2 and t1.p3
	if ((t1.p1.x == t2.p1.x && t1.p1.y == t2.p1.y) || (t1.p1.x == t2.p2.x && t1.p1.y == t2.p2.y) || (t1.p1.x == t2.p3.x && t1.p1.y == t2.p3.y))
	{
		pointCount++;
		t1p1Share = true;

		//determine which point t1.p1 shares with t2
		if (t1.p1.x == t2.p1.x && t1.p1.x == t2.p1.y)
			t2p1Share = true;
		else if (t1.p1.x == t2.p2.x && t1.p1.x == t2.p2.y)
			t2p2Share = true;
		else
			t2p3Share = true;
	}
	if ((t1.p2.x == t2.p1.x && t1.p2.y == t2.p1.y) || (t1.p2.x == t2.p2.x && t1.p2.y == t2.p2.y) || (t1.p2.x == t2.p3.x && t1.p2.y == t2.p3.y))
	{
		pointCount++;
		t1p2Share = true;

		//determine which point t1.p2 shares with t2
		if (t1.p2.x == t2.p1.x && t1.p2.x == t2.p1.y)
			t2p1Share = true;
		else if (t1.p2.x == t2.p2.x && t1.p2.x == t2.p2.y)
			t2p2Share = true;
		else
			t2p3Share = true;
	}
	if (pointCount == 1 && ((t1.p3.x == t2.p1.x && t1.p3.y == t2.p1.y) || (t1.p3.x == t2.p2.x && t1.p3.y == t2.p2.y) || (t1.p3.x == t2.p3.x && t1.p3.y == t2.p3.y)))
	{
		pointCount++;
		t1p3Share = true;

		//determine which point t1.p3 shares with t2
		if (t1.p3.x == t2.p1.x && t1.p3.x == t2.p1.y)
			t2p1Share = true;
		else if (t1.p3.x == t2.p2.x && t1.p3.x == t2.p2.y)
			t2p2Share = true;
		else
			t2p3Share = true;
	}

	//if exactly two points are shared, we know there is a shared edge, thus find the two new triangles to create
	if (pointCount == 2)
	{
		//calculate the distance between points to find the length of the edges to determine which edge is best
		//if tri 1's shared edge is (p1,p2)
		if (t1p1Share && t1p2Share)
		{
			d1 = pythagorean(t1.p1, t1.p2);
			//if tri 2's shared is (p1,p2)
			if (t2p1Share && t2p2Share)
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p3, t2.p3, t1.p1), dist(t1.p3, t2.p3, t1.p2)))
					return false;

				d2 = pythagorean(t1.p3, t2.p3);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{t1.p1, t1.p3, t2.p3};
					t2 = tri{temp.p2, temp.p3, t2.p3};
					return true;
				}
			}
			//elif tri 2's shared edge is (p1,p3)
			else if (t2p1Share && t2p3Share)
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p3, t2.p2, t1.p1), dist(t1.p3, t2.p2, t1.p2)))
					return false;

				d2 = pythagorean(t1.p3, t2.p2);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p1, t1.p3, t2.p2 };
					t2 = tri{ temp.p2, temp.p3, t2.p2 };
					return true;
				}
			}
			//else tri 2's shared edge is (p2,p3)
			else
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p3, t2.p1, t1.p1), dist(t1.p3, t2.p1, t1.p2)))
					return false;

				d2 = pythagorean(t1.p3, t2.p1);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p1, t1.p3, t2.p1 };
					t2 = tri{ temp.p2, temp.p3, t2.p1 };
					return true;
				}
			}
		}
		//elif tri 1's shared edge is (p1,p3)
		else if (t1p1Share && t1p3Share)
		{
			d1 = pythagorean(t1.p1, t1.p3);
			//if tri 2's shared is (p1,p2)
			if (t2p1Share && t2p2Share)
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p2, t2.p3, t1.p1), dist(t1.p2, t2.p3, t1.p3)))
					return false;

				d2 = pythagorean(t1.p2, t2.p3);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p1, t1.p2, t2.p3 };
					t2 = tri{ temp.p3, temp.p2, t2.p3 };
					return true;
				}
			}
			//elif tri 2's shared edge is (p1,p3)
			else if (t2p1Share && t2p3Share)
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p2, t2.p2, t1.p1), dist(t1.p2, t2.p2, t1.p3)))
					return false;

				d2 = pythagorean(t1.p2, t2.p2);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p1, t1.p2, t2.p2 };
					t2 = tri{ temp.p3, temp.p2, t2.p2 };
					return true;
				}
			}
			//else tri 2's shared edge is (p2,p3)
			else
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p2, t2.p1, t1.p1), dist(t1.p2, t2.p1, t1.p3)))
					return false;

				d2 = pythagorean(t1.p2, t2.p1);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p1, t1.p2, t2.p1 };
					t2 = tri{ temp.p3, temp.p2, t2.p1 };
					return true;
				}
			}
		}
		//else tri 1's shared edge is (p2,p3)
		else
		{
			d1 = pythagorean(t1.p2, t1.p3);
			//if tri 2's shared is (p1,p2)
			if (t2p1Share && t2p2Share)
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p1, t2.p3, t1.p2), dist(t1.p1, t2.p3, t1.p3)))
					return false;

				d2 = pythagorean(t1.p1, t2.p3);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p2, t1.p1, t2.p3 };
					t2 = tri{ temp.p3, temp.p1, t2.p3 };
					return true;
				}
			}
			//elif tri 2's shared edge is (p1,p3)
			else if (t2p1Share && t2p3Share)
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p1, t2.p2, t1.p2), dist(t1.p1, t2.p2, t1.p3)))
					return false;

				d2 = pythagorean(t1.p1, t2.p2);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p2, t1.p1, t2.p2 };
					t2 = tri{ temp.p3, temp.p1, t2.p2 };
					return true;
				}
			}
			//else tri 2's shared edge is (p2,p3)
			else
			{
				//if the points making up the edge are on the same side of the new edge, we don't have a better edge
				if (same_sign(dist(t1.p1, t2.p1, t1.p2), dist(t1.p1, t2.p1, t1.p3)))
					return false;

				d2 = pythagorean(t1.p1, t2.p1);

				if (abs(d2) < abs(d1))
				{
					tri temp = tri{ t1.p1, t1.p2, t1.p3 };
					t1 = tri{ t1.p2, t1.p1, t2.p1 };
					t2 = tri{ temp.p3, temp.p1, t2.p1 };
					return true;
				}
			}
		}
	}

	//otherwise the two triangles do not share an edge
	return false;
}

//this method implements a triangle cleanup algorithm to attempt to reduce the line lengths of shared edges
void tri_cleanup()
{
	int trisCleaned = 0;

	//iterate through all tris
	for (tri t1 : global.tris)
	{
		//then, iterate through all tris and compare the top level tri to the lower level tri
		for (tri t2 : global.tris)
		{
			//if the two tris share an edge, replace them with better tris
			if (check_shared_edge(t1, t2))
			{
				trisCleaned++;
				glutPostRedisplay();
			}
		}
	}

	cout << "Triangles cleaned up: " << trisCleaned << endl;
}

//this method performs a triangulation of all points in the global points vector
//first a convex hull is created, then a mid point is found (closest to centre of screen)
//then each triangle formed from the convex hull to the mid point is trisected
//then the number of points and triangles are printed to the console
void triangulation()
{
	if (global.points.size() < 3)
		return;

	vector<tri>().swap(global.tris); //clear the tris vector, getting rid of its contents and freeing some memory
	convex_hull(); //perform an initial convex hull to create the bounds of the set of points

	//initialize variables for determining the middle most point in the window
	point pMid;
	int minD = INT_MAX;
	int d;

	//iterate through all points and find the point closest to the middle that is not on the edge
	for (const point& p : global.points)
	{
		if (!on_edge(p))
		{
			d = pythagorean(p, point{ global.w / 2, global.h / 2 }); //find distance between current point and centre of screen
			if (d < minD) //if the distance is less than the current min distance, set it as the new min distance
			{
				pMid = p;
				minD = d;
			}
		}
	}

	//if we find no interior point, use the first point from the convex hull
	//doesn't currently ensure that the point is not colinear
	if (minD == INT_MAX)
		pMid = global.edges[0].p1;

	//for all edges, create a triangle to the calculated middle point and call trisect on it
	for (const edge& e : global.edges)
		trisect(tri{ e.p1, e.p2, pMid });

	vector<point>().swap(global.points); //clear the points vector, getting rid of its contents and freeing some memory
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	glutPostRedisplay();

	tri_cleanup(); //clean up the triangles

	cout << "Number of points: " << global.points.size() << endl;
	cout << "Number of triangles created: " << global.tris.size() << endl;
}

//this method draws the points and triangles to the window, showing the work done by the triangulation
void draw()
{
	glClear(GL_COLOR_BUFFER_BIT); //make sure to clear the screen before drawing new points

	glPointSize(3.0); //set the size of the points larger so they are easier to see
	glColor3f(1.0, 1.0, 1.0); //set them to white

	//begin drawing points on screen
	glBegin(GL_POINTS);

	//draw all the points in the global points vector onto the screen
	for (const point& p : global.points)
		glVertex2i(p.x, p.y);

	//stop drawing points
	glEnd();

	//draw all the triangles in the global tris vector onto the screen
	for (const tri& t : global.tris)
	{
		glBegin(GL_LINE_STRIP);
		glVertex2i(t.p1.x, t.p1.y);
		glVertex2i(t.p2.x, t.p2.y);
		glVertex2i(t.p3.x, t.p3.y);
		glEnd();
	}

	//flush the buffer to screen
	glFlush();
}

//this method increments the global n value by 100
//this is used to add 100 more points to the global points vector for generating random points
//it ensures that the number of points will not exceed the size of the coords vector of all possible points
void increment_n()
{
	//if the global n is already at the size of the coordinate vector, inform the user and don't increment
	if (global.n == global.coords.size())
	{
		cout << "No more than " << global.n << " points allowed at current screen size!" << endl;
		return;
	}

	//check if the program has already shuffled the coords vector, if not, shuffle it
	if (!global.shuffled)
	{
		random_shuffle(global.coords.begin(), global.coords.end()); //shuffle the coordinate vector
		global.shuffled = true;
		global.n -= 10;
	}

	//add 10 points to the global points vector
	global.n += 10;
	for (int i = global.points.size(); i < global.n; i++)
		global.points.push_back(point{ global.coords[i].x, global.coords[i].y }); //add the new point to the point vector

	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	vector<tri>().swap(global.tris); //clear the tris vector, getting rid of its contents and freeing some memory

	glutPostRedisplay(); //redisplay the window
}

//this method sets the global mouse draw bool to its opposite, and clears the global points vector is mouseDraw was set to true
void set_mouse_draw()
{
	global.mouseDraw = !global.mouseDraw;
	if (global.mouseDraw)
		vector<point>().swap(global.points);
}

/*glut keyboard function*/
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 0x1B:
	case'q':
	case 'Q':
		exit(0);
		break;
	case 'r':
	case 'R':
		random();
		break;
	case 'm':
	case 'M':
		set_mouse_draw();
		break;
	case 't':
	case 'T':
		triangulation();
		break;
	case 'l':
	case 'L':
		lattice();
		break;
	case 'a':
	case 'A':
		increment_n();
		break;
	case 'c':
	case 'C':
		tri_cleanup();
	}
}//keyboard

//Glut menu callback function
void menuFunc(int value)
{
	switch (value)
	{
	case MENU_QUIT:
		exit(0);
		break;
		break;
	case MENU_RANDOM:
		random();
		break;
	case MENU_LATTICE:
		lattice();
		break;
	case MENU_TRIANGULATION:
		triangulation();
		break;
	case MENU_INCREMENT:
		increment_n();
		break;
	case MENU_MOUSE:
		set_mouse_draw();
		break;
	}

	glutPostRedisplay();
}//menuFunc

//show the keys for actions in the terminal
void show_keys()
{
	printf("Q:quit\nR:random\nM:mouse selection\nA:Add 100 points\nL:lattice\nT:triangulation\n");
}

//Glut menu set up
void init_menu()
{
	//menu for overall menu options
	int main_menu = glutCreateMenu(&menuFunc);
	glutAddMenuEntry("Random Points", MENU_RANDOM);
	glutAddMenuEntry("Add 10 Points", MENU_INCREMENT);
	glutAddMenuEntry("Mouse Points", MENU_MOUSE);
	glutAddMenuEntry("Lattice Points", MENU_LATTICE);
	glutAddMenuEntry("Triangulation", MENU_TRIANGULATION);
	glutAddMenuEntry("Quit", MENU_QUIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//initialize the vector for coordinates
void initializeVector()
{
	//initialize coordinate vector, going through all possible combinations of x and y (making sure points are only drawn within the visual bounds of the screen!)
	for (int x = 1; x < global.w - 9; x++)
		for (int y = 1; y < global.h - 9; y++)
			global.coords.push_back(point{ x, y }); //add the point to the global vector of points
}

//what runs the whole show
int main(int argc, char** argv)
{
	srand(static_cast <unsigned> (time(0))); //set a new random seed

	global.w = 1000; //width and height set to 1000 x 800
	global.h = 800;
	global.n = 10; //set default number of points to 100 (maximum based on window size - 774,200)
	initializeVector(); //initialize the coordinate vectors

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);

	glutInitWindowSize(global.w, global.h);
	glutCreateWindow("2D Triangulation");
	glShadeModel(GL_SMOOTH);
	glutKeyboardFunc(keyboard);
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(0, global.w, 0, global.h);

	init_menu();
	show_keys();

	glutMouseFunc(mouse);
	glutDisplayFunc(draw);

	glutMainLoop();
}