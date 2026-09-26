#ifndef OPENTISSUE_CORE_CONTAINERS_GRID_IO_GRID_METAIMAGE_WRITE_H
#define OPENTISSUE_CORE_CONTAINERS_GRID_IO_GRID_METAIMAGE_WRITE_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace OpenTissue
{
  namespace grid
  {

    namespace detail
    {

      /**
      * Mapping from a C++ type onto the MetaImage element type that describes it.
      *
      * Only the types MetaImage actually defines are listed. Instantiating this for any
      * other type fails to compile, which is the intended behaviour: silently writing a
      * header that misdescribes the data would produce a file that loads and is wrong.
      */
      template<typename T> struct MetaImageElementType;

      template<> struct MetaImageElementType<char>           { static char const * name() { return "MET_CHAR";   } };
      template<> struct MetaImageElementType<unsigned char>  { static char const * name() { return "MET_UCHAR";  } };
      template<> struct MetaImageElementType<short>          { static char const * name() { return "MET_SHORT";  } };
      template<> struct MetaImageElementType<unsigned short> { static char const * name() { return "MET_USHORT"; } };
      template<> struct MetaImageElementType<int>            { static char const * name() { return "MET_INT";    } };
      template<> struct MetaImageElementType<unsigned int>   { static char const * name() { return "MET_UINT";   } };
      template<> struct MetaImageElementType<float>          { static char const * name() { return "MET_FLOAT";  } };
      template<> struct MetaImageElementType<double>         { static char const * name() { return "MET_DOUBLE"; } };

      /**
      * The raw block is written in host byte order, so the header has to say which that is.
      */
      inline bool is_big_endian()
      {
        union { unsigned int value; unsigned char bytes[4]; } probe;
        probe.value = 1u;
        return probe.bytes[0] == 0u;
      }

      /**
      * MetaImage resolves ElementDataFile relative to the header, so that field has to hold
      * a bare file name rather than whatever path the caller supplied.
      */
      inline std::string base_name(std::string const & path)
      {
        std::string::size_type const slash = path.find_last_of("/\\");
        return (slash == std::string::npos) ? path : path.substr(slash + 1);
      }

    } // namespace detail

    /**
    * Write a grid as a MetaImage, for ParaView and other VTK-based tools.
    *
    * Two files are produced: a short text header, filename.mhd, and the voxel data as
    * filename.raw. Opening the .mhd in ParaView gives a correctly sized and positioned
    * volume, ready for a contour or volume-rendering filter.
    *
    * This is the compact route. grid_matlab_write.h is more convenient for small grids one
    * wants to inspect numerically, but writes text and does not scale; here the data is a
    * plain binary dump, so a 256^3 grid of floats is 64 MB rather than several hundred.
    *
    * The grid's own layout is used unchanged. OpenTissue indexes as (k*J + j)*I + i, so i
    * varies fastest, which is exactly the order MetaImage expects for DimSize = I J K.
    *
    * @param filename   Where to write. A trailing ".mhd" is optional and will not be
    *                   doubled; the raw file is named to match.
    * @param grid       The grid to write.
    *
    * @return           True if both files were written.
    */
    template <typename grid_type>
    inline bool metaimage_write(std::string const & filename, grid_type const & grid)
    {
      typedef typename grid_type::value_type value_type;

      std::string stem = filename;
      if(stem.size() > 4u && stem.compare(stem.size() - 4u, 4u, ".mhd") == 0)
        stem = stem.substr(0, stem.size() - 4u);

      std::string const header_name = stem + ".mhd";
      std::string const raw_name    = stem + ".raw";

      std::ofstream header(header_name.c_str());
      if(!header)
      {
        std::cerr << "metaimage_write(): could not open " << header_name << std::endl;
        return false;
      }

      header << "ObjectType = Image\n"
             << "NDims = 3\n"
             << "BinaryData = True\n"
             << "BinaryDataByteOrderMSB = " << (detail::is_big_endian() ? "True" : "False") << "\n"
             << "CompressedData = False\n"
             << "TransformMatrix = 1 0 0 0 1 0 0 0 1\n"
             << "Offset = "
             << grid.min_coord()(0) << " "
             << grid.min_coord()(1) << " "
             << grid.min_coord()(2) << "\n"
             << "CenterOfRotation = 0 0 0\n"
             << "ElementSpacing = " << grid.dx() << " " << grid.dy() << " " << grid.dz() << "\n"
             << "DimSize = " << grid.I() << " " << grid.J() << " " << grid.K() << "\n"
             << "ElementType = " << detail::MetaImageElementType<value_type>::name() << "\n"
             << "ElementDataFile = " << detail::base_name(raw_name) << "\n";
      header.close();

      std::ofstream raw(raw_name.c_str(), std::ios::binary);
      if(!raw)
      {
        std::cerr << "metaimage_write(): could not open " << raw_name << std::endl;
        return false;
      }
      raw.write(reinterpret_cast<char const *>(grid.data()),
                static_cast<std::streamsize>(grid.size() * sizeof(value_type)));
      raw.close();

      // Silent on success: this is typically called once per frame of an animation.
      return true;
    }

  } // namespace grid
} // namespace OpenTissue

//OPENTISSUE_CORE_CONTAINERS_GRID_IO_GRID_METAIMAGE_WRITE_H
#endif
