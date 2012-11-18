#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h> 
#include <Foundation/Foundation.h>

/* -----------------------------------------------------------------------------
   Step 1
   Set the UTI types the importer supports
  
   Modify the CFBundleDocumentTypes entry in Info.plist to contain
   an array of Uniform Type Identifiers (UTI) for the LSItemContentTypes 
   that your importer can handle
  
   ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
   Step 2 
   Implement the GetMetadataForFile function
  
   Implement the GetMetadataForFile function below to scrape the relevant
   metadata from your document and return it as a CFDictionary using standard keys
   (defined in MDItem.h) whenever possible.
   ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
   Step 3 (optional) 
   If you have defined new attributes, update the schema.xml file
  
   Edit the schema.xml file to include the metadata keys that your importer returns.
   Add them to the <allattrs> and <displayattrs> elements.
  
   Add any custom types that your importer requires to the <attributes> element
  
   <attribute name="com_mycompany_metadatakey" type="CFString" multivalued="true"/>
  
   ----------------------------------------------------------------------------- */



/* -----------------------------------------------------------------------------
    Get metadata attributes from file
   
   This function's job is to extract useful information your file format supports
   and return it as a dictionary
   ----------------------------------------------------------------------------- */

Boolean GetMetadataForFile(void* thisInterface, 
			   CFMutableDictionaryRef attributes, 
			   CFStringRef contentTypeUTI,
			   CFStringRef pathToFile)
{
        /* Pull any available metadata from the file at the specified path */
        /* Return the attribute keys and attribute values in the dict */
        /* Return TRUE if successful, FALSE if there was no data provided */
    Boolean success=NO;
    NSString *theContent;
    NSAutoreleasePool *pool;

        // Don't assume that there is an autorelease pool around the calling of this function.
    pool = [[NSAutoreleasePool alloc] init];
        // load the document at the specified location
    theContent=[[NSString alloc] initWithContentsOfFile:(NSString *)pathToFile
								//usedEncoding:(NSStringEncoding *)NULL
								encoding:NSUTF8StringEncoding
								error:(NSError **)NULL];
    if (theContent)
    {
	[(NSMutableDictionary *)attributes setObject:theContent
                                forKey:(NSString *)kMDItemTextContent];

            // return YES so that the attributes are imported
	success=YES;

            // release the loaded document
	[theContent release];
    }
    [pool release];
    return success;
}
