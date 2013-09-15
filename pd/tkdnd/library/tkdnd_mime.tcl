#
# tkdnd_unix.tcl --
# 
#    This file implements some utility procedures that are used by the TkDND
#    package.
#
# This software is copyrighted by:
# George Petasis, National Centre for Scientific Research "Demokritos",
# Aghia Paraskevi, Athens, Greece.
# e-mail: petasis@iit.demokritos.gr
#
# The following terms apply to all files associated
# with the software unless explicitly disclaimed in individual files.
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.
# 
# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
# FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
# DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
# MODIFICATIONS.
#

namespace eval mime {
};# namespace mime

## ===========================================================================
##   URL section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::urls
# ----------------------------------------------------------------------------
proc mime::urls {} {
};# mime::urls

# ----------------------------------------------------------------------------
#  Command mime::setUrls
# ----------------------------------------------------------------------------
proc mime::setUrls {urls {type "text/uri-list"}} {
  variable data
  variable transfer_format
  variable transfer_data
  
  set data($type) $url
  ## We transfer 
};# mime::setUrls

# ----------------------------------------------------------------------------
#  Command mime::hasUrls
# ----------------------------------------------------------------------------
proc mime::hasUrls {{type "text/uri-list"}} {
  return [hasFormat $type]
};# mime::hasUrls

## ===========================================================================
##   Text section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::text
# ----------------------------------------------------------------------------
proc mime::text {} {
};# mime::text

# ----------------------------------------------------------------------------
#  Command mime::setText
# ----------------------------------------------------------------------------
proc mime::setText {} {
};# mime::setText

# ----------------------------------------------------------------------------
#  Command mime::hasText
# ----------------------------------------------------------------------------
proc mime::hasText {} {
};# mime::hasText

## ===========================================================================
##   Text section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::html
# ----------------------------------------------------------------------------
proc mime::html {} {
};# mime::html

# ----------------------------------------------------------------------------
#  Command mime::setHtml
# ----------------------------------------------------------------------------
proc mime::setHtml {} {
};# mime::setHtml

# ----------------------------------------------------------------------------
#  Command mime::hasHtml
# ----------------------------------------------------------------------------
proc mime::hasHtml {} {
};# mime::hasHtml

## ===========================================================================
##   Colour section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::colorData
# ----------------------------------------------------------------------------
proc mime::colorData {} {
};# mime::colorData

# ----------------------------------------------------------------------------
#  Command mime::setColorData
# ----------------------------------------------------------------------------
proc mime::setColorData {} {
};# mime::setColorData

# ----------------------------------------------------------------------------
#  Command mime::hasColorData
# ----------------------------------------------------------------------------
proc mime::hasColorData {} {
};# mime::hasColorData

## ===========================================================================
##   Image section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::imageData
# ----------------------------------------------------------------------------
proc mime::imageData {} {
};# mime::imageData

# ----------------------------------------------------------------------------
#  Command mime::setImageData
# ----------------------------------------------------------------------------
proc mime::setImageData {} {
};# mime::setImageData

# ----------------------------------------------------------------------------
#  Command mime::hasImageData
# ----------------------------------------------------------------------------
proc mime::hasImageData {} {
};# mime::hasImageData

## ===========================================================================
##   Binary section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::
# ----------------------------------------------------------------------------
proc mime::data {} {
};# mime::data

# ----------------------------------------------------------------------------
#  Command mime::setData
# ----------------------------------------------------------------------------
proc mime::setData {format data} {
  variable data
  ## Remove previous data...
  array unset data $format
  set data($format) $data
};# mime::setData

# ----------------------------------------------------------------------------
#  Command mime::hasData
# ----------------------------------------------------------------------------
proc mime::hasData {} {
};# mime::hasData

## ===========================================================================
##   Utilities section
## ===========================================================================

# ----------------------------------------------------------------------------
#  Command mime::clear
# ----------------------------------------------------------------------------
proc mime::clear {} {
  variable data
  array unset data *
};# mime::clear

# ----------------------------------------------------------------------------
#  Command mime::hasFormat
# ----------------------------------------------------------------------------
proc mime::hasFormat {mimetype} {
  variable data
  return [info exists data($mimetype)]
};# mime::hasFormat

# ----------------------------------------------------------------------------
#  Command mime::formats
# ----------------------------------------------------------------------------
proc mime::formats {} {
};# mime::formats

# ----------------------------------------------------------------------------
#  Command mime::retrieveData
# ----------------------------------------------------------------------------
proc mime::retrieveData {} {
};# mime::retrieveData

# ----------------------------------------------------------------------------
#  Command mime::registerSelectionHandler
# ----------------------------------------------------------------------------
proc mime::registerSelectionHandler {} {
};# mime::registerSelectionHandler

# ----------------------------------------------------------------------------
#  Command mime::unregisterSelectionHandler
# ----------------------------------------------------------------------------
proc mime::unregisterSelectionHandler {} {
};# mime::unregisterSelectionHandler
