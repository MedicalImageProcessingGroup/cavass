/*============================================================================
  KWSys - Kitware System Library
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef itksys_Directory_hxx
#define itksys_Directory_hxx

#include <itksys/Configure.h>
#include <itksys/stl/string>

/* Define these macros temporarily to keep the code readable.  */
#if !defined (KWSYS_NAMESPACE) && !itksys_NAME_IS_KWSYS
# define kwsys_stl itksys_stl
# define kwsys_ios itksys_ios
#endif

namespace itksys
{

class DirectoryInternals;

/** \class Directory
 * \brief Portable directory/filename traversal.
 *
 * Directory provides a portable way of finding the names of the files
 * in a system directory.
 *
 * Directory currently works with Windows and Unix operating systems.
 */
class itksys_EXPORT Directory
{
public:
  Directory();
  ~Directory();

  /**
   * Load the specified directory and load the names of the files
   * in that directory. 0 is returned if the directory can not be
   * opened, 1 if it is opened.
   */
  bool Load(const kwsys_stl::string&);

  /**
   * Return the number of files in the current directory.
   */
  unsigned long GetNumberOfFiles() const;

  /**
   * Return the number of files in the specified directory.
   * A higher performance static method.
   */
  static unsigned long GetNumberOfFilesInDirectory(const kwsys_stl::string&);

  /**
   * Return the file at the given index, the indexing is 0 based
   */
  const char* GetFile(unsigned long) const;

  /**
   * Return the path to Open'ed directory
   */
  const char* GetPath() const;

  /**
   * Clear the internal structure. Used internally at beginning of Load(...)
   * to clear the cache.
   */
  void Clear();

private:
  // Private implementation details.
  DirectoryInternals* Internal;

  Directory(const Directory&);  // Not implemented.
  void operator=(const Directory&);  // Not implemented.
}; // End Class: Directory

} // namespace itksys

/* Undefine temporary macros.  */
#if !defined (KWSYS_NAMESPACE) && !itksys_NAME_IS_KWSYS
# undef kwsys_stl
# undef kwsys_ios
#endif

#endif
