import condor.*;

import java.io.*;
import java.net.URL;
import java.util.*;

public class SubmitJob
   {

   public static void main(String[] args)
      throws Exception
      {
      URL scheddLocation = new URL("http://localhost:0");

      // Get a handle on a schedd we can make SOAP call on.
      CondorScheddLocator scheddLocator = new CondorScheddLocator();
      CondorScheddPortType schedd = scheddLocator.getcondorSchedd(scheddLocation);

      String filesToSend[] = {"TestFile"};

      SOAPScheddApiHelper.submitJobHelper(schedd, null, -1, -1, "nobody", UniverseType.VANILLA, "/bin/cp","-f TestFile Testfile.worked", "OpSys==\"LINUX\"", null, filesToSend);
      }

   }
