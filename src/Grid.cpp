#include "StdAfx.h"
#include "Grid.h"

/** Constructor */
Grid::Grid()
{ }

/** Destructor */
Grid::~Grid()
{ }

/** Initialize grid */
void Grid::init(const Aabb& aabb, float cellSize)
{
  aabb_ = aabb;
  Vector3 vecShift = (aabb_.max_-aabb_.min_)*0.1f; //slightly bigger aabb
  aabb_.min_-=vecShift;
  aabb_.max_+=vecShift;

  cellSize_ = cellSize;
  Vector3 diff(aabb_.max_-aabb_.min_);
  dimX_ = static_cast<int>(ceil(diff.x()/cellSize_));
  dimY_ = static_cast<int>(ceil(diff.y()/cellSize_));
  dimZ_ = static_cast<int>(ceil(diff.z()/cellSize_));
  if(dimX_<=0) dimX_ = 1;
  if(dimY_<=0) dimY_ = 1;
  if(dimZ_<=0) dimZ_ = 1;
  size_ = 1+(dimX_*dimY_*dimZ_);
  if (size_ > static_cast<int>(iVerts_.size())) {
    iVerts_.resize(size_);
  }
  for (int i=0; i<size_; i++) {
    iVerts_[i].clear();
  }
  dimXY_ = dimX_*dimY_;
}

/** Build the grid */
void Grid::build(Mesh *mesh, const std::vector<int> &iVerts)
{
  VertexVector &vertices = mesh->getVertices();
  int nbVerts = iVerts.size();
  Vector3& min = aabb_.min_;
  for(int i = 0; i<nbVerts; ++i)
  {
    int iVert = iVerts[i];
    Vertex &v = vertices[iVert];
    Vector3 diff(v-min);
    int index = getIndex(static_cast<int>(diff.x()/cellSize_), static_cast<int>(diff.y()/cellSize_), static_cast<int>(diff.z()/cellSize_));
    if(index<0)
      index = 0;
    else if(index>=size_)
      index = size_ - 1;
    iVerts_[index].push_back(iVert);
  }
}
/** Return neighboring vertices */
void Grid::getNeighborhood(const Vector3& v, std::vector<int>& iNearVerts)
{
  const Vector3& min = aabb_.min_;
  const Vector3 diff(v-min);
  int indX = static_cast<int>(diff.x()/cellSize_);
  int indY = static_cast<int>(diff.y()/cellSize_);
  int indZ = static_cast<int>(diff.z()/cellSize_);

  if(indX<0) indX = 0;
  else if(indX>=dimX_) indX = dimX_-1;

  if(indY<0) indY = 0;
  else if(indY>=dimY_) indY = dimY_-1;

  if(indZ<0) indZ = 0;
  else if(indZ>=dimZ_) indZ = dimZ_-1;

  int xStart = -1;
  int yStart = -1;
  int zStart = -1;

  int xEnd = 1;
  int yEnd = 1;
  int zEnd = 1;

  if(indX<=0) xStart = 0;
  if(indX>=(dimX_-1)) xEnd = 0;

  if(indY<=0) yStart = 0;
  if(indY>=(dimY_-1)) yEnd = 0;

  if(indZ<=0) zStart = 0;
  if(indZ>=(dimZ_-1)) zEnd = 0;

  iNearVerts.clear();

  for(int iX = xStart; iX<=xEnd; ++iX)
  {
    for(int iY = yStart; iY<=yEnd; ++iY)
    {
      for(int iZ = zStart; iZ<=zEnd; ++iZ)
      {
        const std::vector<int> &iVerts = iVerts_[getIndex(indX + iX, indY + iY, indZ + iZ)];
        iNearVerts.insert(iNearVerts.end(), iVerts.begin(), iVerts.end());
      }
    }
  }
}

