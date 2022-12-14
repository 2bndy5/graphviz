#pragma once

#include <string>
#include <string_view>

#include "svg_element.h"

/**
 * @brief The GraphvizEdge class represents a Graphviz edge according to the DOT
 * language
 */

class GraphvizEdge {
public:
  GraphvizEdge() = delete;
  explicit GraphvizEdge(SVG::SVGElement &svg_g_element);

  /// Return the bounding box of the edge
  SVG::SVGRect bbox() const;
  /// Return the center of the edge's bounding box
  SVG::SVGPoint center() const;
  /// Return the 'edgeop' according to the DOT language specification. Note that
  /// this is not the same as the 'id' attribute of an edge.
  std::string_view edgeop() const;
  /// Return a non-mutable reference to the SVG `g` element corresponding to the
  /// edge
  const SVG::SVGElement &svg_g_element() const;

private:
  /// The 'edgeop' according to the DOT language specification
  std::string m_edgeop;
  /// The SVG `g` element corresponding to the edge
  SVG::SVGElement &m_svg_g_element;
};
