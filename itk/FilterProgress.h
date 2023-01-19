/*
  Copyright 1993-2008 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/


#include  "itkCommand.h"

/** \brief The following section of code implements a Command observer
 *  that will monitor the progress of the registration process.
 */
class FilterProgress : public itk::Command
{
public :
    typedef  FilterProgress           Self;
    typedef  itk::Command             Superclass;
    typedef  itk::SmartPointer<Self>  Pointer;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    itkNewMacro ( Self );
protected :
    FilterProgress ( void ) { }
public :
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void Execute ( itk::Object* caller, const itk::EventObject& event ) {
        Execute( (const itk::Object *)caller, event );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void Execute ( const itk::Object* object, const itk::EventObject&  event )
    {
/*
        OptimizerPointer optimizer = dynamic_cast< OptimizerPointer >( object );
        if (typeid(event) != typeid(itk::ProgressEvent))    return;
        std::cout << optimizer->GetCurrentIteration() << "   "
                      << optimizer->GetValue() << "   "
                      << optimizer->GetCurrentPosition();
*/
        if (typeid(event) != typeid(itk::ProgressEvent))    return;
        
        const itk::ProcessObject*  po
            = dynamic_cast<const itk::ProcessObject*>(object);
        int  percent = (int)((po->GetProgress() * 100) + 0.5);
        std::cout << object->GetNameOfClass() << ": " << percent << "%"
                  << std::endl;
    }
};

