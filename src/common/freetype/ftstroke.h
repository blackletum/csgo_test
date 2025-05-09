﻿/***************************************************************************/
/*                                                                         */
/*  ftstroke.h                                                             */
/*                                                                         */
/*    FreeType path stroker (specification).                               */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005, 2006 by                              */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FT_STROKE_H__
#define __FT_STROKE_H__

#include <ft2build.h>
#include FT_OUTLINE_H
#include FT_GLYPH_H


FT_BEGIN_HEADER


 /************************************************************************
  *
  * @section:
  *    glyph_stroker
  *
  * @title:
  *    Glyph Stroker
  *
  * @abstract:
  *    Generating bordered and stroked glyphs.
  *
  * @description:
  *    This component generates stroked outlines of a given vectorial
  *    glyph.  It also allows you to retrieve the `outside' and/or the
  *    `inside' borders of the stroke.
  *
  *    This can be useful to generate `bordered' glyph, i.e., glyphs
  *    displayed with a coloured (and anti-aliased) border around their
  *    shape.
  */


 /**************************************************************
  *
  * @type:
  *   FT_Stroker
  *
  * @description:
  *   Opaque handler to a path stroker object.
  */
  typedef struct FT_StrokerRec_*  FT_Stroker;


  /**************************************************************
   *
   * @enum:
   *   FT_Stroker_LineJoin
   *
   * @description:
   *   These values determine how two joining lines are rendered
   *   in a stroker.
   *
   * @values:
   *   FT_STROKER_LINEJOIN_ROUND ::
   *     Used to render rounded line joins.  Circular arcs are used
   *     to join two lines smoothly.
   *
   *   FT_STROKER_LINEJOIN_BEVEL ::
   *     Used to render beveled line joins; i.e., the two joining lines
   *     are extended until they intersect.
   *
   *   FT_STROKER_LINEJOIN_MITER ::
   *     Same as beveled rendering, except that an additional line
   *     break is added if the angle between the two joining lines
   *     is too closed (this is useful to avoid unpleasant spikes
   *     in beveled rendering).
   */
  typedef enum
  {
    FT_STROKER_LINEJOIN_ROUND = 0,
    FT_STROKER_LINEJOIN_BEVEL,
    FT_STROKER_LINEJOIN_MITER

  } FT_Stroker_LineJoin;


  /**************************************************************
   *
   * @enum:
   *   FT_Stroker_LineCap
   *
   * @description:
   *   These values determine how the end of opened sub-paths are
   *   rendered in a stroke.
   *
   * @values:
   *   FT_STROKER_LINECAP_BUTT ::
   *     The end of lines is rendered as a full stop on the last
   *     point itself.
   *
   *   FT_STROKER_LINECAP_ROUND ::
   *     The end of lines is rendered as a half-circle around the
   *     last point.
   *
   *   FT_STROKER_LINECAP_SQUARE ::
   *     The end of lines is rendered as a square around the
   *     last point.
   */
  typedef enum
  {
    FT_STROKER_LINECAP_BUTT = 0,
    FT_STROKER_LINECAP_ROUND,
    FT_STROKER_LINECAP_SQUARE

  } FT_Stroker_LineCap;


  /**************************************************************
   *
   * @enum:
   *   FT_StrokerBorder
   *
   * @description:
   *   These values are used to select a given stroke border
   *   in @FT_Stroker_GetBorderCounts and @FT_Stroker_ExportBorder.
   *
   * @values:
   *   FT_STROKER_BORDER_LEFT ::
   *     Select the left border, relative to the drawing direction.
   *
   *   FT_STROKER_BORDER_RIGHT ::
   *     Select the right border, relative to the drawing direction.
   *
   * @note:
   *   Applications are generally interested in the `inside' and `outside'
   *   borders.  However, there is no direct mapping between these and the
   *   `left' and `right' ones, since this really depends on the glyph's
   *   drawing orientation, which varies between font formats.
   *
   *   You can however use @FT_Outline_GetInsideBorder and
   *   @FT_Outline_GetOutsideBorder to get these.
   */
  typedef enum
  {
    FT_STROKER_BORDER_LEFT = 0,
    FT_STROKER_BORDER_RIGHT

  } FT_StrokerBorder;


  /**************************************************************
   *
   * @function:
   *   FT_Outline_GetInsideBorder
   *
   * @description:
   *   Retrieve the @FT_StrokerBorder value corresponding to the
   *   `inside' borders of a given outline.
   *
   * @input:
   *   outline ::
   *     The source outline handle.
   *
   * @return:
   *   The border index.  @FT_STROKER_BORDER_LEFT for empty or invalid
   *   outlines.
   */
  FT_EXPORT( FT_StrokerBorder )
  FT_Outline_GetInsideBorder( FT_Outline*  outline );


  /**************************************************************
   *
   * @function:
   *   FT_Outline_GetOutsideBorder
   *
   * @description:
   *   Retrieve the @FT_StrokerBorder value corresponding to the
   *   `outside' borders of a given outline.
   *
   * @input:
   *   outline ::
   *     The source outline handle.
   *
   * @return:
   *   The border index.  @FT_STROKER_BORDER_LEFT for empty or invalid
   *   outlines.
   */
  FT_EXPORT( FT_StrokerBorder )
  FT_Outline_GetOutsideBorder( FT_Outline*  outline );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_New
   *
   * @description:
   *   Create a new stroker object.
   *
   * @input:
   *   library ::
   *     FreeType library handle.
   *
   * @output:
   *   astroker ::
   *     A new stroker object handle.  NULL in case of error.
   *
   * @return:
   *    FreeType error code.  0 means success.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_New( FT_Library   library,
                  FT_Stroker  *astroker );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_Set
   *
   * @description:
   *   Reset a stroker object's attributes.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   radius ::
   *     The border radius.
   *
   *   line_cap ::
   *     The line cap style.
   *
   *   line_join ::
   *     The line join style.
   *
   *   miter_limit ::
   *     The miter limit for the FT_STROKER_LINEJOIN_MITER style,
   *     expressed as 16.16 fixed point value.
   *
   * @note:
   *   The radius is expressed in the same units that the outline
   *   coordinates.
   */
  FT_EXPORT( void )
  FT_Stroker_Set( FT_Stroker           stroker,
                  FT_Fixed             radius,
                  FT_Stroker_LineCap   line_cap,
                  FT_Stroker_LineJoin  line_join,
                  FT_Fixed             miter_limit );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_Rewind
   *
   * @description:
   *   Reset a stroker object without changing its attributes.
   *   You should call this function before beginning a new
   *   series of calls to @FT_Stroker_BeginSubPath or
   *   @FT_Stroker_EndSubPath.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   */
  FT_EXPORT( void )
  FT_Stroker_Rewind( FT_Stroker  stroker );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_ParseOutline
   *
   * @description:
   *   A convenience function used to parse a whole outline with
   *   the stroker.  The resulting outline(s) can be retrieved
   *   later by functions like @FT_Stroker_GetCounts and @FT_Stroker_Export.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   outline ::
   *     The source outline.
   *
   *   opened ::
   *     A boolean.  If 1, the outline is treated as an open path instead
   *     of a closed one.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   If `opened' is 0 (the default), the outline is treated as a closed
   *   path, and the stroker will generate two distinct `border' outlines.
   *
   *   If `opened' is 1, the outline is processed as an open path, and the
   *   stroker will generate a single `stroke' outline.
   *
   *   This function calls @FT_Stroker_Rewind automatically.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_ParseOutline( FT_Stroker   stroker,
                           FT_Outline*  outline,
                           FT_Bool      opened );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_BeginSubPath
   *
   * @description:
   *   Start a new sub-path in the stroker.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   to ::
   *     A pointer to the start vector.
   *
   *   open ::
   *     A boolean.  If 1, the sub-path is treated as an open one.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   This function is useful when you need to stroke a path that is
   *   not stored as an @FT_Outline object.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_BeginSubPath( FT_Stroker  stroker,
                           FT_Vector*  to,
                           FT_Bool     open );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_EndSubPath
   *
   * @description:
   *   Close the current sub-path in the stroker.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   You should call this function after @FT_Stroker_BeginSubPath.
   *   If the subpath was not `opened', this function will `draw' a
   *   single line segment to the start position when needed.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_EndSubPath( FT_Stroker  stroker );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_LineTo
   *
   * @description:
   *   `Draw' a single line segment in the stroker's current sub-path,
   *   from the last position.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   to ::
   *     A pointer to the destination point.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   You should call this function between @FT_Stroker_BeginSubPath and
   *   @FT_Stroker_EndSubPath.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_LineTo( FT_Stroker  stroker,
                     FT_Vector*  to );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_ConicTo
   *
   * @description:
   *   `Draw' a single quadratic BÃ©zier in the stroker's current sub-path,
   *   from the last position.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   control ::
   *     A pointer to a BÃ©zier control point.
   *
   *   to ::
   *     A pointer to the destination point.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   You should call this function between @FT_Stroker_BeginSubPath and
   *   @FT_Stroker_EndSubPath.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_ConicTo( FT_Stroker  stroker,
                      FT_Vector*  control,
                      FT_Vector*  to );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_CubicTo
   *
   * @description:
   *   `Draw' a single cubic BÃ©zier in the stroker's current sub-path,
   *   from the last position.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   control1 ::
   *     A pointer to the first BÃ©zier control point.
   *
   *   control2 ::
   *     A pointer to second BÃ©zier control point.
   *
   *   to ::
   *     A pointer to the destination point.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   You should call this function between @FT_Stroker_BeginSubPath and
   *   @FT_Stroker_EndSubPath.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_CubicTo( FT_Stroker  stroker,
                      FT_Vector*  control1,
                      FT_Vector*  control2,
                      FT_Vector*  to );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_GetBorderCounts
   *
   * @description:
   *   Call this function once you have finished parsing your paths
   *   with the stroker.  It will return the number of points and
   *   contours necessary to export one of the `border' or `stroke'
   *   outlines generated by the stroker.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   border ::
   *     The border index.
   *
   * @output:
   *   anum_points ::
   *     The number of points.
   *
   *   anum_contours ::
   *     The number of contours.
   *
   * @return:
   *   FreeType error code.  0 means success.
   *
   * @note:
   *   When an outline, or a sub-path, is `closed', the stroker generates
   *   two independent `border' outlines, named `left' and `right'.
   *
   *   When the outline, or a sub-path, is `opened', the stroker merges
   *   the `border' outlines with caps.  The `left' border receives all
   *   points, while the `right' border becomes empty.
   *
   *   Use the function @FT_Stroker_GetCounts instead if you want to
   *   retrieve the counts associated to both borders.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_GetBorderCounts( FT_Stroker        stroker,
                              FT_StrokerBorder  border,
                              FT_UInt          *anum_points,
                              FT_UInt          *anum_contours );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_ExportBorder
   *
   * @description:
   *   Call this function after @FT_Stroker_GetBorderCounts to
   *   export the corresponding border to your own @FT_Outline
   *   structure.
   *
   *   Note that this function will append the border points and
   *   contours to your outline, but will not try to resize its
   *   arrays.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   border ::
   *     The border index.
   *
   *   outline ::
   *     The target outline handle.
   *
   * @note:
   *   Always call this function after @FT_Stroker_GetBorderCounts to
   *   get sure that there is enough room in your @FT_Outline object to
   *   receive all new data.
   *
   *   When an outline, or a sub-path, is `closed', the stroker generates
   *   two independent `border' outlines, named `left' and `right'
   *
   *   When the outline, or a sub-path, is `opened', the stroker merges
   *   the `border' outlines with caps. The `left' border receives all
   *   points, while the `right' border becomes empty.
   *
   *   Use the function @FT_Stroker_Export instead if you want to
   *   retrieve all borders at once.
   */
  FT_EXPORT( void )
  FT_Stroker_ExportBorder( FT_Stroker        stroker,
                           FT_StrokerBorder  border,
                           FT_Outline*       outline );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_GetCounts
   *
   * @description:
   *   Call this function once you have finished parsing your paths
   *   with the stroker.  It returns the number of points and
   *   contours necessary to export all points/borders from the stroked
   *   outline/path.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   * @output:
   *   anum_points ::
   *     The number of points.
   *
   *   anum_contours ::
   *     The number of contours.
   *
   * @return:
   *   FreeType error code.  0 means success.
   */
  FT_EXPORT( FT_Error )
  FT_Stroker_GetCounts( FT_Stroker  stroker,
                        FT_UInt    *anum_points,
                        FT_UInt    *anum_contours );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_Export
   *
   * @description:
   *   Call this function after @FT_Stroker_GetBorderCounts to
   *   export the all borders to your own @FT_Outline structure.
   *
   *   Note that this function will append the border points and
   *   contours to your outline, but will not try to resize its
   *   arrays.
   *
   * @input:
   *   stroker ::
   *     The target stroker handle.
   *
   *   outline ::
   *     The target outline handle.
   */
  FT_EXPORT( void )
  FT_Stroker_Export( FT_Stroker   stroker,
                     FT_Outline*  outline );


  /**************************************************************
   *
   * @function:
   *   FT_Stroker_Done
   *
   * @description:
   *   Destroy a stroker object.
   *
   * @input:
   *   stroker ::
   *     A stroker handle.  Can be NULL.
   */
  FT_EXPORT( void )
  FT_Stroker_Done( FT_Stroker  stroker );


  /**************************************************************
   *
   * @function:
   *   FT_Glyph_Stroke
   *
   * @description:
   *   Stroke a given outline glyph object with a given stroker.
   *
   * @inout:
   *   pglyph ::
   *     Source glyph handle on input, new glyph handle on output.
   *
   * @input:
   *   stroker ::
   *     A stroker handle.
   *
   *   destroy ::
   *     A Boolean.  If 1, the source glyph object is destroyed
   *     on success.
   *
   * @return:
   *    FreeType error code.  0 means success.
   *
   * @note:
   *   The source glyph is untouched in case of error.
   */
  FT_EXPORT( FT_Error )
  FT_Glyph_Stroke( FT_Glyph    *pglyph,
                   FT_Stroker   stroker,
                   FT_Bool      destroy );


  /**************************************************************
   *
   * @function:
   *   FT_Glyph_StrokeBorder
   *
   * @description:
   *   Stroke a given outline glyph object with a given stroker, but
   *   only return either its inside or outside border.
   *
   * @inout:
   *   pglyph ::
   *     Source glyph handle on input, new glyph handle on output.
   *
   * @input:
   *   stroker ::
   *     A stroker handle.
   *
   *   inside ::
   *     A Boolean.  If 1, return the inside border, otherwise
   *     the outside border.
   *
   *   destroy ::
   *     A Boolean.  If 1, the source glyph object is destroyed
   *     on success.
   *
   * @return:
   *    FreeType error code.  0 means success.
   *
   * @note:
   *   The source glyph is untouched in case of error.
   */
  FT_EXPORT( FT_Error )
  FT_Glyph_StrokeBorder( FT_Glyph    *pglyph,
                         FT_Stroker   stroker,
                         FT_Bool      inside,
                         FT_Bool      destroy );

 /* */

FT_END_HEADER

#endif /* __FT_STROKE_H__ */


/* END */


/* Local Variables: */
/* coding: utf-8    */
/* End:             */
