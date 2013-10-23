#ifndef __MESH_H__
#define __MESH_H__

#include <cinder/gl/gl.h>
#include <cinder/Thread.h>
#include "DataTypes.h"
#include <string>
#include <algorithm>
#include <stdint.h>
#include <list>
#include "Tools.h"
#include "Triangle.h"
#include "Vertex.h"
#include "State.h"
#include "omp.h"
#include "Geometry.h"
#include "GLBuffer.h"
#include "Brush.h"

class Octree;

/**
* Mesh
* @author St�phane GINIER
*/
class Mesh
{
public:
	static const float globalScale_; //for precision issue...
	static int stateMask_; //for history

public:
	Mesh();
	~Mesh();
	TriangleVector& getTriangles();
	VertexVector& getVertices();
	std::vector<Octree*>& getLeavesUpdate();
	Triangle& getTriangle(int i);
  const Triangle& getTriangle(int i) const;
  Vertex& getVertex(int i);
	const Vertex& getVertex(int i) const;
	int getNbTriangles() const;
	int getNbVertices() const;
	Vector3 getCenter() const;
	Octree* getOctree() const;
	float getScale() const;
	void setIsSelected(bool);
	Matrix4x4& getTransformation();
	Matrix4x4 getInverseTransformation() const;
  void setRotationVelocity(float vel);
  void updateRotation(float deltaTime);
  const Vector3& getRotationOrigin() const;
  const Vector3& getRotationAxis() const;
  float getRotationVelocity() const;

	std::vector<int> getTrianglesFromVertices(const std::vector<int> &iVerts);
	std::vector<int> getVerticesFromTriangles(const std::vector<int> &iTris);
	void expandTriangles(std::vector<int> &iTris, int nRing);
	void expandVertices(std::vector<int> &iVerts, int nRing);
	void computeRingVertices(int iVert);
	void getVerticesInsideSphere(const Vector3& point, float radiusWorldSquared, std::vector<int>& result);
  void getVerticesInsideBrush(const Brush& brush, std::vector<int>& result);

	Vector3 getTriangleCenter(int iTri) const;
	void moveTo(const Vector3& destination);
	void setTransformation(const Matrix4x4& matTransform);

	void draw(GLint vertex, GLint normal, GLint color);
  void drawVerticesOnly(GLint vertex);
  void drawOctree() const;
	void initMesh();

	void updateMesh(const std::vector<int> &iTris, const std::vector<int> &iVerts);
  void updateGPUBuffers();

	std::vector<int> subdivide(std::vector<int> &iTris,std::vector<int> &iVerts,float inradiusMaxSquared);
	void triangleSubdivision(int iTri);

	void checkLeavesUpdate();

	//undo-redo
	TriangleVector& getTrianglesState();
	VertexVector& getVerticesState();
	void startPushState();
	void pushState(const std::vector<int> &iTris, const std::vector<int> &iVerts);
	void undo();
	void redo();
  void handleUndoRedo();
	void recomputeOctree(const Aabb &aabbSplit);

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:

	void updateOctree(const std::vector<int> &iTris);
	void updateNormals(const std::vector<int> &iVerts);
	float angleTri(int iTri, int iVer);
	void updateTransformation();
  void initIndexVBO();
  void initVertexVBO();
  void reinitVerticesBuffer();
  void reinitIndicesBuffer();
  void performUndo();
  void performRedo();

	VertexVector vertices_; //vertices
	TriangleVector triangles_; //triangles
  GLint verticesBufferCount_;
  GLint indicesBufferCount_;
	GLBuffer verticesBuffer_; //vertices buffer (openGL)
	GLBuffer normalsBuffer_; //normals buffer (openGL)
	GLBuffer indicesBuffer_; //indexes (openGL)
  GLBuffer colorsBuffer_;
  bool needVerticesRefresh_;
  bool needIndicesRefresh_;
	Vector3 center_; //center of mesh
	float scale_; //scale
	Octree *octree_; //octree
	Matrix4x4 matTransform_; //transformation matrix of the mesh
	GLfloat matTransformArray_[16]; //transformation matrix of the mesh (openGL)
	std::vector<Octree*> leavesUpdate_; //leaves of the octree to check
  bool undoPending_;
  bool redoPending_;

	//undo-redo
	std::list<State> undo_; //undo actions
	std::list<State> redo_; //redo actions
	std::list<State>::iterator undoIte_; //iterator to undo
	bool beginIte_; //end of undo action

  Vector3 rotationOrigin_;
  Vector3 rotationAxis_;
  float rotationVelocity_;
  float curRotation_;

  struct IndexUpdate {
    int idx;
    GLuint indices[3];
  };

  struct VertexUpdate {
    int idx;
    Vector3 pos;
    Vector3 color;
    Vector3 normal;
  };

  typedef std::vector<IndexUpdate, Eigen::aligned_allocator<IndexUpdate> > IndexUpdateVector;
  typedef std::vector<VertexUpdate, Eigen::aligned_allocator<VertexUpdate> > VertexUpdateVector;

  std::deque<IndexUpdateVector> indexUpdates_;
  std::deque<VertexUpdateVector> vertexUpdates_;
  boost::mutex bufferMutex_;
};

#endif /*__MESH_H__*/
