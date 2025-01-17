/*!
 * VisualModels which have data.
 */
#pragma once

#include <vector>
#include <morph/Vector.h>
#include <morph/VisualModel.h>
#include <morph/ColourMap.h>
#include <morph/Scale.h>

namespace morph {

    //! Class for VisualModels that visualize data of type T. T is probably float or
    //! double, but may be integer types, too.
    template <typename T>
    class VisualDataModel : public VisualModel
    {
    public:
        VisualDataModel()
            : morph::VisualModel::VisualModel() {}

        VisualDataModel (GLuint sp, const Vector<float> _offset)
            : morph::VisualModel::VisualModel (sp, _offset) {}

        //! Deconstructor should *not* deallocate data - client code should do that
        ~VisualDataModel() {}

        //! Reset the autoscaled flags so that the next time data is transformed by
        //! the Scale objects they will autoscale again (assuming they have
        //! do_autoscale set true).
        void clearAutoscale()
        {
            this->zScale.autoscaled = false;
            this->colourScale.autoscaled = false;
            this->vectorScale.autoscaled = false;
        }

        void clearAutoscaleZ() { this->zScale.autoscaled = false; }
        void clearAutoscaleColour() { this->colourScale.autoscaled = false; }
        void clearAutoscaleVector() { this->vectorScale.autoscaled = false; }

        void setZScale (const Scale<T, float>& zscale) { this->zScale = zscale; }
        void setCScale (const Scale<T, float>& cscale) { this->colourScale = cscale; }
        void setScalarData (const std::vector<T>* _data) { this->scalarData = _data; }
        void setDataCoords (std::vector<Vector<float>>* _coords) { this->dataCoords = _coords; }

        void updateZScale (const Scale<T, float>& zscale)
        {
            this->zScale = zscale;
            this->reinit();
        }

        void updateCScale (const Scale<T, float>& cscale)
        {
            this->colourScale = cscale;
            this->reinit();
        }

        void setVectorScale (const Scale<Vector<T>>& vscale)
        {
            this->vectorScale = vscale;
            this->reinit();
        }

        void setColourMap (ColourMapType _cmt, const float _hue = 0.0f)
        {
            this->cm.setHue (_hue);
            this->cm.setType (_cmt);
        }

        //! Update the scalar data
        void updateData (const std::vector<T>* _data)
        {
            this->scalarData = _data;
            this->reinit();
        }

        //! Update the scalar data with an associated z-scaling
        void updateData (const std::vector<T>* _data, const Scale<T, float>& zscale)
        {
            this->scalarData = _data;
            this->zScale = zscale;
            this->reinit();
        }

        //! Update the scalar data, along with both the z-scaling and the colour-scaling
        void updateData (const std::vector<T>* _data, const Scale<T, float>& zscale, const Scale<T, float>& cscale)
        {
            this->scalarData = _data;
            this->zScale = zscale;
            this->colourScale = cscale;
            this->reinit();
        }

        //! Update coordinate data and scalar data along with z-scaling for scalar data
        virtual void updateData (std::vector<Vector<float>>* _coords, const std::vector<T>* _data,
                                 const Scale<T, float>& zscale)
        {
            this->dataCoords = _coords;
            this->scalarData = _data;
            this->zScale = zscale;
            this->reinit();
        }

        //! Update coordinate data and scalar data along with z- and colour-scaling for scalar data
        virtual void updateData (std::vector<Vector<float>>* _coords, const std::vector<T>* _data,
                                 const Scale<T, float>& zscale, const Scale<T, float>& cscale)
        {
            this->dataCoords = _coords;
            this->scalarData = _data;
            this->zScale = zscale;
            this->colourScale = cscale;
            this->reinit();
        }

        //! Update just the coordinate data
        virtual void updateCoords (std::vector<Vector<float>>* _coords)
        {
            this->dataCoords = _coords;
            this->reinit();
        }

        //! Update the vector data (for plotting quiver plots)
        void updateData (const std::vector<Vector<float>>* _vectors)
        {
            this->vectorData = _vectors;
            this->reinit();
        }

        //! Update both coordinate and vector data
        void updateData (std::vector<Vector<float>>* _coords, const std::vector<Vector<T>>* _vectors)
        {
            this->dataCoords = _coords;
            this->vectorData = _vectors;
            this->reinit();
        }

        //! Re-initialize the buffers. Client code might have appended to
        //! vertexPositions/Colors/Normals and indices before calling this method.
        void reinit_buffers()
        {
            morph::gl::Util::checkError (__FILE__, __LINE__);
            // Now re-set up the VBOs
#ifdef CAREFULLY_UNBIND_AND_REBIND // Experimenting with better buffer binding.
            glBindVertexArray (this->vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->vbos[idxVBO]);
#endif
            int sz = this->indices.size() * sizeof(VBOint);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sz, this->indices.data(), GL_STATIC_DRAW);
            this->setupVBO (this->vbos[posnVBO], this->vertexPositions, gl::posnLoc);
            this->setupVBO (this->vbos[normVBO], this->vertexNormals, gl::normLoc);
            this->setupVBO (this->vbos[colVBO], this->vertexColors, gl::colLoc);

#ifdef CAREFULLY_UNBIND_AND_REBIND
            glBindVertexArray(0);
            morph::gl::Util::checkError (__FILE__, __LINE__);
#endif
        }

        //! Re-create the model - called after updating data
        void reinit()
        {
            // Fixme: Better not to clear, then repeatedly pushback here:
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();
            this->initializeVertices();
            this->reinit_buffers();
        }

        void setZeroGrid (const bool _zerogrid) { this->zerogrid = _zerogrid; }

        //! All data models use a a colour map. Change the type/hue of this colour map
        //! object to generate different types of map.
        ColourMap<float> cm;

        //! A Scaling function for the colour map. Perhaps a Scale class contains a
        //! colour map? If not, then this scale might well be autoscaled. Applied to scalarData.
        Scale<T, float> colourScale;

        //! A scale to scale (or autoscale) scalarData. This might be used to set z
        //! locations of data coordinates based on scalarData. The scaling may
        Scale<T, float> zScale;

        //! A scaling function for the vectorData. This will scale the lengths of the
        //! vectorData.
        Scale<Vector<T>> vectorScale;

        //! The data to visualize. T may simply be float or double, or, if the
        //! visualization is of directional information, such as in a quiver plot,
        const std::vector<T>* scalarData = (const std::vector<T>*)0;

        //! A container for vector data to visualize.
        const std::vector<Vector<T>>* vectorData = (const std::vector<Vector<T>>*)0;

        //! The coordinates at which to visualize data, if appropriate (e.g. scatter
        //! graph, quiver plot). Note fixed type of float, which is suitable for
        //! OpenGL coordinates. Not const as child code may resize or update content.
        std::vector<Vector<float>>* dataCoords = (std::vector<Vector<float>>*)0;

        //! Graph data coordinates. Different from dataCoords, because it's a vector of
        //! vectors of pointers to data, with one pointer for each graph in the
        //! model. Not const, too.
        std::vector<std::vector<Vector<float>>*> graphDataCoords;
    };

} // namespace morph
