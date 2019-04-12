/*
Copyright (c) 2018-2019 Onur Rauf Bingol

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "rw3dm.h"


void initializeRwExt()
{
    ON::Begin();
}

void finalizeRwExt()
{
    ON::End();
}

void extractCurveData(const ON_Geometry* geometry, Config &cfg, Json::Value &data)
{
    // We know that "geometry" is a curve object
    const ON_Curve *curve = (ON_Curve *)geometry;

    // Try to get the NURBS form of the curve object
    ON_NurbsCurve nurbsCurve;
    if (curve->NurbsCurve(&nurbsCurve))
    {
        // Get rational
        data["rational"] = nurbsCurve.IsRational();

        // Get degree
        data["degree"] = nurbsCurve.Degree();

        // Get knot vector
        Json::Value knotVector;
        knotVector.append(nurbsCurve.SuperfluousKnot(false));
        for (int idx = 0; idx < nurbsCurve.KnotCount(); idx++)
            knotVector[idx] = nurbsCurve.Knot(idx);
        knotVector.append(nurbsCurve.SuperfluousKnot(true));
        data["knotvector"] = knotVector;

        Json::Value controlPoints;
        Json::Value points;

        // Get control points
        int numCoords = nurbsCurve.IsRational() ? nurbsCurve.CVSize() - 1 : nurbsCurve.CVSize();
        for (int idx = 0; idx < nurbsCurve.CVCount(); idx++)
        {
            double *vertex = nurbsCurve.CV(idx);
            Json::Value point;
            for (int c = 0; c < numCoords; c++)
                point[c] = vertex[c];
            points[idx] = point;
        }
        controlPoints["points"] = points;

        // Check if the NURBS curve is rational
        if (nurbsCurve.IsRational())
        {
            Json::Value weights;
            // Get weights
            for (int idx = 0; idx < nurbsCurve.CVCount(); idx++)
            {
                double *vertex = nurbsCurve.CV(idx);
                weights[idx] = vertex[numCoords - 1];
            }
            controlPoints["weights"] = weights;
        }
        data["control_points"] = controlPoints;
    }
}

void extractSurfaceData(const ON_Geometry* geometry, Config &cfg, Json::Value &data)
{
    // We know that "geometry" is a surface object
    const ON_Surface *surface = (ON_Surface *)geometry;

    // Try to get the NURBS form of the surface object
    ON_NurbsSurface nurbsSurface;
    if (surface->NurbsSurface(&nurbsSurface))
    {
        // Get rational
        data["rational"] = nurbsSurface.IsRational();

        // Get degrees
        data["degree_u"] = nurbsSurface.Degree(0);
        data["degree_v"] = nurbsSurface.Degree(1);

        // Get knot vectors
        Json::Value knotVectorU;
        knotVectorU.append(nurbsSurface.SuperfluousKnot(0, false));
        for (int idx = 0; idx < nurbsSurface.KnotCount(0); idx++)
            knotVectorU[idx] = nurbsSurface.Knot(0, idx);
        knotVectorU.append(nurbsSurface.SuperfluousKnot(0, true));
        data["knotvector_u"] = knotVectorU;

        Json::Value knotVectorV;
        knotVectorV.append(nurbsSurface.SuperfluousKnot(1, false));
        for (int idx = 0; idx < nurbsSurface.KnotCount(1); idx++)
            knotVectorV[idx] = nurbsSurface.Knot(1, idx);
        knotVectorV.append(nurbsSurface.SuperfluousKnot(1, true));
        data["knotvector_v"] = knotVectorV;

        Json::Value controlPoints;
        Json::Value points;

        // Get control points
        int numCoords = nurbsSurface.IsRational() ? nurbsSurface.CVSize() - 1 : nurbsSurface.CVSize();
        int sizeU = nurbsSurface.CVCount(0);
        int sizeV = nurbsSurface.CVCount(1);
        for (int idxU = 0; idxU < sizeU; idxU++)
        {
            for (int idxV = 0; idxV < sizeV; idxV++)
            {
                unsigned int idx = idxV + (idxU * sizeV);
                double *vertex = nurbsSurface.CV(idxU, idxV);
                Json::Value point;
                for (int c = 0; c < numCoords; c++)
                    point[c] = vertex[c];
                points[idx] = point;
            }
        }
        controlPoints["points"] = points;

        // Check if the NURBS surface is rational
        if (nurbsSurface.IsRational())
        {
            Json::Value weights;
            // Get weights
            for (int idxU = 0; idxU < sizeU; idxU++)
            {
                for (int idxV = 0; idxV < sizeV; idxV++)
                {
                    unsigned int idx = idxV + (idxU * sizeV);
                    double *vertex = nurbsSurface.CV(idxU, idxV);
                    weights[idx] = vertex[numCoords - 1];
                }
            }
            controlPoints["weights"] = weights;
        }
        data["size_u"] = sizeU;
        data["size_v"] = sizeV;
        data["control_points"] = controlPoints;
    }
}

void extractBrepData(const ON_Geometry* geometry, Config &cfg, Json::Value &data)
{
    // We know that "geometry" is a BRep object
    ON_Brep *brep = (ON_Brep *)geometry;

    // Standardize relationships of all surfaces, edges and trims in the BRep object
    brep->Standardize();

    // Delete unnecessary curves and surfaces after "standardize"
    brep->Compact();

    // Face loop
    unsigned int faceIdx = 0;
    unsigned int surfIdx = 0;
    ON_BrepFace *brepFace;
    while (brepFace = brep->Face(faceIdx))
    {
        const ON_Surface *faceSurf = brepFace->SurfaceOf();
        if (faceSurf)
        {
            Json::Value surfData;
            extractSurfaceData(faceSurf, cfg, surfData);
            data[surfIdx] = surfData;
            surfIdx++;
        }
        faceIdx++;
    }
}

void constructCurveData(ONX_Model &model, Config &cfg, Json::Value &data)
{
    // Spatial dimension
    int dimension = data["control_points"]["points"][0].size();

    // Can only work with 2- and 3-dimensional curves
    if (dimension > 3 || dimension < 2)
    {
        if (cfg.warnings())
            std::cout << "[WARNING]: OpenNURBS supports 2- and 3-dimensional curves only. Skipping..." << std::endl;
        return;
    }
 
    // Number of control points
    int numCtrlpts = data["control_points"]["points"].size();

    // Create OpenNURBS curve instance
    ON_NurbsCurve nurbsCurve(
        dimension,
        true,
        data["degree"].asInt() + 1,
        numCtrlpts
    );

    // Set knot vector
    for (int idx = 0; idx < nurbsCurve.KnotCount(); idx++)
        nurbsCurve.SetKnot(idx, data["knotvector"][idx].asDouble());

    // Set control points
    for (int idx = 0; idx < nurbsCurve.CVCount(); idx++)
    {
        Json::Value cptData = data["control_points"]["points"][idx];
        double w;
        if (data["rational"] == 0)
            w = 1.0;
        else
            w = data["control_points"]["weights"][idx].asDouble();
        ON_4dPoint cpt(
            cptData[0].asDouble() * w,
            cptData[1].asDouble() * w,
            (dimension == 2) ? 0.0 : cptData[2].asDouble() * w,
            w
        );
        nurbsCurve.SetCV(idx, cpt);
    }

    // Add curve to the model
    model.AddModelGeometryComponent(&nurbsCurve, nullptr);
}

void constructSurfaceData(ONX_Model &model, Config &cfg, Json::Value &data)
{
    // Spatial dimension
    int dimension = data["control_points"]["points"][0].size();

    // Can only work with 3-dimensional surfaces
    if (dimension > 3 || dimension < 2)
    {
        if (cfg.warnings())
            std::cout << "[WARNING]: OpenNURBS supports 3-dimensional surfaces only. Skipping..." << std::endl;
        return;
    }

    // Number of control points
    int sizeU = data["size_u"].asInt();
    int sizeV = data["size_v"].asInt();

    // Create OpenNURBS surface instance
    ON_NurbsSurface nurbsSurface(
        dimension,
        true,
        data["degree_u"].asInt() + 1,
        data["degree_v"].asInt() + 1,
        sizeU,
        sizeV
    );

    // Set knot vectors
    for (int idx = 0; idx < nurbsSurface.KnotCount(0); idx++)
        nurbsSurface.SetKnot(0, idx, data["knotvector_u"][idx].asDouble());
    for (int idx = 0; idx < nurbsSurface.KnotCount(1); idx++)
        nurbsSurface.SetKnot(1, idx, data["knotvector_v"][idx].asDouble());

    // Set control points
    for (int idxU = 0; idxU < nurbsSurface.CVCount(0); idxU++)
    {
        for (int idxV = 0; idxV < nurbsSurface.CVCount(1); idxV++)
        {
            unsigned int idx = idxV + (idxU * sizeV);
            Json::Value cptData = data["control_points"]["points"][idx];
            double w;
            if (data["rational"] == 0)
                w = 1.0;
            else
                w = data["control_points"]["weights"][idx].asDouble();
            ON_4dPoint cpt(
                cptData[0].asDouble() * w,
                cptData[1].asDouble() * w,
                cptData[2].asDouble() * w,
                w
            );
            nurbsSurface.SetCV(idxU, idxV, cpt);
        }
    }

    // Each surface (and the trim curves) should belong to a BRep object
    ON_Brep brep;
    brep.NewFace(nurbsSurface);

    // Add BRep to the model
    model.AddModelGeometryComponent(&brep, nullptr);
}
