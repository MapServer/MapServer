#!/home/nsavard/fgs/bin/python
##!c:/python25/python.exe

import cgi
import difflib
import logging
import os
import os.path
import string
import subprocess
import sys

import ConfigParser

config = ConfigParser.ConfigParser()
config.readfp(open("config.cfg"))
python_logging_file = config.get("Logging", "PythonLoggingFile")
logging_flag = config.getboolean("Logging", "PythonLogging")


def main():

    function_name = "main:  "

    if not os.path.isdir(os.path.dirname(python_logging_file)):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The logging file: "
            + python_logging_file
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The logging file:  "
            + python_logging_file
            + " does not exist.  Check the configuration file."
        )

    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s %(levelname)s %(message)s",
        filename=python_logging_file,
        filemode="a",
    )

    if logging_flag == False:
        logging.disable("debug")

    print("Content-type: text/html")
    print("")
    print(
        """

    <html>
    <head>
    <title>msautotest viewer</title>
    </head>
    <body>
      <h1>Auto Compare Maps</h1>
      <a href="./">Back to msautotest root directory</a>
      <form method=POST>

        <INPUT TYPE=HIDDEN NAME="Position" VALUE="">
        <INPUT TYPE=HIDDEN NAME="Direction" VALUE="">
        <INPUT TYPE=HIDDEN NAME="Directory" VALUE="">
        <INPUT TYPE=HIDDEN NAME="URL" VALUE="">

    <script language="JavaScript" type="text/javascript">

      function PrevPage(nResults, nPos, szDir, szURL)
      {
          //alert('prevpage');
          document.forms[0].Direction.value="PREV";
          document.forms[0].Position.value = parseInt(nPos) - parseInt(nResults);
          document.forms[0].Directory.value=szDir;
          document.forms[0].URL.value=szURL;
          document.forms[0].submit();
      }

      function NextPage(nResults, nPos, szDir, szURL)
      {
          //alert('nextpage');
          document.forms[0].Direction.value="NEXT";
          document.forms[0].Position.value = parseInt(nPos) + parseInt(nResults);
          document.forms[0].Directory.value=szDir;
          document.forms[0].URL.value=szURL;
          document.forms[0].submit();
      }

      var bSwitching = 0;

      function startSwitching(szImageName, szImage1, szImage2)
      {
          bSwitching = 0;
          switch2image2(szImageName, szImage1, szImage2);
      }

      function stopSwitching()
      {
          bSwitching =1;
      }

      function switch2image1(szImageName, szImage1, szImage2) 
      {
          oImageName2Display = document.getElementById(szImageName);
          oImageName2Display.value = szImage1;

          if(!bSwitching)
          {
              document.images[szImageName].src = szImage1;
              setTimeout("switch2image2('" + szImageName + "','" + szImage1 + "','" + szImage2 +"')", 1000);
          }
      }

      function switch2image2(szImageName, szImage1, szImage2) 
      {
          oImageName2Display = document.getElementById(szImageName);
          oImageName2Display.value = szImage2;


          if(!bSwitching)
          {
              document.images[szImageName].src = szImage2;
              setTimeout("switch2image1('" + szImageName + "','" + szImage1 + "','" + szImage2 +"')", 1000);

          }
      }

    </script>

    <br>

    <table border=1>"""
    )

    rmTmpPngImages()

    results_per_page = int(config.get("Report", "ResultsPerPage"))

    GET_parameters = cgi.FieldStorage()

    for key in GET_parameters.keys():
        logging.debug("key=" + str(key) + ", value=" + str(GET_parameters[key]))

    if "Direction" in GET_parameters:
        direction = GET_parameters.getfirst("Direction", "")
    else:
        direction = ""

    logging.debug("direction=" + direction)

    if "Position" in GET_parameters:
        position = GET_parameters.getfirst("Position", "")
    else:
        position = 0

    logging.debug("position=" + str(position))

    if "Directory" in GET_parameters:
        directory = GET_parameters.getfirst("Directory", "")
        ##directory = GET_parameters['Directory'].value
    else:
        directory = ""
    logging.debug("directory=" + directory)

    if "URL" in GET_parameters:
        URL = GET_parameters.getfirst("URL", "")
        # URL = GET_parameters['URL'].value
    else:
        URL = ""

    # Opening and reading directory
    count = 0
    lines_list = []

    logging.debug("getcwd=" + os.getcwd())

    ## Build result & expected directories/URLs
    current_directory = os.getcwd()
    current_directory = current_directory.replace("\\", "/")
    logging.debug(function_name + "current directory=" + current_directory)
    actual_directory = directory + "/result/"
    expected_directory = directory + "/expected/"

    actual_URL = URL + "/result/"
    expected_URL = URL + "/expected/"

    logging.debug(function_name + "directory=" + directory)
    logging.debug(function_name + "actual directory=" + actual_directory)
    logging.debug(function_name + "expected directory=" + expected_directory)
    logging.debug(function_name + "actual URL=" + actual_URL)
    logging.debug(function_name + "expected URL=" + expected_URL)

    if os.path.isdir(current_directory):
        for file in os.listdir(actual_directory):
            if string.find(file, ".aux") != -1:
                continue
            else:
                logging.debug(function_name + "filename: " + file)
                line = file + " "
                line += actual_directory + file + " "
                line += actual_URL + file + " "
                line += expected_directory + file + " "
                line += expected_URL + file

            # logging.debug( function_name + lines)
            lines_list.append(line)

    # logging.debug( function_name + str(lines_list))
    count = len(lines_list)

    # Control "NEXT" and "PREVIOUS" buttons appearance
    if direction == "NEXT":
        prev_button = (
            '<a href="javascript:PrevPage('
            + str(results_per_page)
            + ","
            + str(position)
            + ", '"
            + str(directory)
            + "' , '"
            + str(URL)
            + "')\">PREV</a>"
        )
        if (int(position) + results_per_page) < count:
            next_button = (
                '<a href="javascript:NextPage('
                + str(results_per_page)
                + ","
                + str(position)
                + ", '"
                + str(directory)
                + "' , '"
                + str(URL)
                + "')\">NEXT</a>"
            )

        else:
            next_button = "<a>NEXT</a>"
    elif direction == "PREV":
        next_button = (
            '<a href="javascript:NextPage('
            + str(results_per_page)
            + ","
            + str(position)
            + ", '"
            + str(directory)
            + "' , '"
            + str(URL)
            + "')\">NEXT</a>"
        )

        if (int(position) - results_per_page) >= 0:
            prev_button = (
                '<a href="javascript:PrevPage('
                + str(results_per_page)
                + ","
                + str(position)
                + ", '"
                + str(directory)
                + "' , '"
                + str(URL)
                + "')\">PREV</a>"
            )
        else:
            prev_button = "<a>PREV</a>"

    else:
        prev_button = "<a>PREV</a>"
        if (int(position) + results_per_page) < count:
            next_button = (
                '<a href="javascript:NextPage('
                + str(results_per_page)
                + ","
                + str(position)
                + ","
                + "'"
                + str(directory)
                + "' , '"
                + str(URL)
                + "')\">NEXT</a>"
            )
        else:
            next_button = "<a>NEXT</a>"

    logging.debug(function_name + "direction=" + direction)
    logging.debug(function_name + "count=" + str(count))
    logging.debug(function_name + "pos=" + str(position))
    logging.debug(function_name + "number of results=" + str(results_per_page))

    # Prepare result output
    i = int(position)
    parameters_list_list = []
    output_list = []
    diff_output_list_list = []

    # parameters_list_list value
    # File name   Actual dir Actual URL Expected dir Expected URL
    #   [0]          [1]        [2]         [3]         [4]
    while i < (results_per_page + int(position)) and i < count:
        file_info_list = lines_list[i].split(" ")
        # logging.debug(function_name + str(file_info_list))
        parameters_list_list.append(file_info_list)
        i += 1
    logging.debug(function_name + str(parameters_list_list))

    print(
        "<tr><td><table border=1 width=100%><tr><td>"
        + prev_button
        + "</td><td></td><td>"
        + next_button
        + "</td></tr></table></td></tr>\n"
    )

    i = 0
    for parameters_list in parameters_list_list:
        # Process xml, dat or html files
        if (
            string.find(parameters_list[0], ".xml") != -1
            or string.find(parameters_list[0], ".dat") != -1
            or string.find(parameters_list[0], ".html") != -1
            or string.find(parameters_list[0], ".js") != -1
        ):

            logging.debug(function_name + str(parameters_list[0]))

            print(
                "<tr><td><table border=1 width=100%><tr><td>Diff output expected vs actual ---File: "
                + parameters_list[0]
                + "</td></tr>\n"
            )

            expected_fp = open(parameters_list[3], "r")
            expected_list = expected_fp.readlines()
            result_fp = open(parameters_list[1], "r")
            result_list = result_fp.readlines()

            output_list = difflib.unified_diff(
                expected_list, result_list, "Expected", "Result"
            )
            diff_output_list_list.append("".join(output_list))

            # logging.debug(function_name + str(diff_output_list_list[len(diff_output_list_list) - 1]))

            print(
                "<tr><td><TEXTAREA COLS=175 ROWS=25x>value="
                + diff_output_list_list[len(diff_output_list_list) - 1]
                + "</TEXTAREA></td></tr></table></td></tr>"
            )

        elif (
            string.find(parameters_list[0], ".tif") != -1
            or string.find(parameters_list[0], ".png") != -1
        ):
            # Process tiff, png files
            logging.debug(function_name + str(parameters_list[0]))

            # Is it a tiff file:  II or MM?
            # Test with the file in the actual directory:  parameters_list[1]
            file_header = open(parameters_list[1], "rb").readline(16)

            if "\x49\x49\x2A\x00" in file_header or "\x49\x49\x00\x2A" in file_header:

                logging.debug(function_name + "geotiff")

                expected_PNG_image_URL = tiff2png(parameters_list[3])
                actual_PNG_image_URL = tiff2png(parameters_list[1])
                # expected_PNG_image_URL = parameters_list[4]
                # actual_PNG_image_URL = parameters_list[2]
                print(
                    "<tr><td><table border=1 width=100%><tr><td>File:"
                    + parameters_list[0]
                    + "</td><td>GeoTiff converted to Png.</td></tr></tr></table></td></tr>"
                )

            else:
                expected_PNG_image_URL = parameters_list[4]
                actual_PNG_image_URL = parameters_list[2]
                print(
                    "<tr><td><table border=1 width=100%><tr><td>File:"
                    + parameters_list[0]
                    + "</td><td></td></tr></tr></table></td></tr>"
                )

            # Display switching buttons
            print(
                '<tr><td><table border=1 width=100%><tr><td>Expected</td><td>Actual</td><td><table><tr><td><INPUT TYPE=button NAME="Start" onclick="startSwitching(\'imagefliper'
                + str(i)
                + "', '"
                + actual_PNG_image_URL
                + "', '"
                + expected_PNG_image_URL
                + '\')" >Start switching</td><td><INPUT TYPE=button NAME="Stop" onclick="stopSwitching()" >Stop switching</td><td>image:<input type="text" id="imagefliper'
                + str(i)
                + '" name="imagefliper'
                + str(i)
                + '" value="'
                + actual_PNG_image_URL
                + '" size="30"></td></tr></table></td></tr>\n'
            )

            # Display images
            print(
                '<tr><td><img width="400" height="300" src="'
                + expected_PNG_image_URL
                + '"></td><td><img width="400" height="300" src="'
                + actual_PNG_image_URL
                + '"></td><td><img name="imagefliper'
                + str(i)
                + '" width="400" height="300" src="'
                + actual_PNG_image_URL
                + '"></td></tr></table><td><tr>\n'
            )

        else:
            continue

        i += 1  # End of for loop

    print(
        "<tr><td><table border=1 width=100%><tr><td>"
        + prev_button
        + "</td><td></td><td>"
        + next_button
        + "</td></tr></table></td></tr>\n"
    )

    print(
        """

          </table>
          <br>
        </form>
      </body>
    </html>"""
    )


## Convert GeoTiff to PNG file in order to be able to see the image in the
## browser
def tiff2png(tiff_file):
    function_name = "tiff2png:  "

    ## Checking if path is found
    if not os.path.isdir(os.path.dirname(python_logging_file)):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The logging file: "
            + python_logging_file
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The logging file:  "
            + python_logging_file
            + "does not exist.  Check the configuration file."
        )

    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s %(levelname)s %(message)s",
        filename=python_logging_file,
        filemode="a",
    )

    if logging_flag == False:
        logging.disable("debug")

    out_file = config.get("Logging", "ShellOutputFile")
    ## Checking if path is found
    if not os.path.isdir(os.path.dirname(out_file)):
        print("Content-type: text/html")
        print()
        print(
            "<html><pre>"
            + function_name
            + ":The output log file: "
            + out_file
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The output log file:  "
            + out_file
            + " does not exist.  Check the configuration file."
        )

    out_fp = open(out_file, "a")

    err_file = config.get("Logging", "ShellErrorFile")
    ## Checking if path is found
    if not os.path.isdir(os.path.dirname(err_file)):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The error log file: "
            + err_file
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The error log file:  "
            + err_file
            + " does not exist.  Check the configuration file."
        )

    err_fp = open(err_file, "a")

    tmp_path = config.get("Applications", "TmpDir")
    ## Checking if path is found
    if not os.path.isdir(tmp_path):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The tmp directory: "
            + tmp_path
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The tmp directory:  "
            + tmp_path
            + " does not exist.  Check the configuration file."
        )

    tmp_web = config.get("Applications", "TmpWeb")
    ## Checking if path is found
    if not os.path.isdir(tmp_web):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The tmp Web URL: "
            + tmp_web
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The tmp Web URL:  "
            + tmp_path
            + " does not exist.  Check the configuration file."
        )

    dirname = os.path.dirname(tiff_file)
    logging.debug(function_name + "dirname=" + dirname)
    ## Result or expected
    sub_path = os.path.basename(dirname)
    logging.debug(function_name + "su_path=" + sub_path)

    basename = os.path.basename(tiff_file)
    imagename_list = os.path.splitext(basename)
    filename = imagename_list[0] + "_msautotest_" + sub_path
    logging.debug(function_name + "filename=" + filename)
    PNG_file = tmp_path + filename + ".png"
    PNG_URL = tmp_web + filename + ".png"
    logging.debug(function_name + "PNG_URL=" + PNG_URL)

    image_converter = config.get("Applications", "ImageConverterCmd")

    ## Command has parameters so we need to get the executable only
    image_converter_parameters_list = image_converter.split(" ")

    ## Checking if the path exist
    if not os.path.isdir(os.path.dirname(image_converter_parameters_list[0])):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The image converter command: "
            + image_converter_parameters_list[0]
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The image converter command:  "
            + image_converter_parameters_list[0]
            + "does not exist.  Check the configuration file."
        )

    ## Executing conversion
    cmd = image_converter + " png " + tiff_file + " " + PNG_file

    logging.debug(function_name + "cmd=" + cmd)
    subprocess.call(cmd, shell=True, stdout=out_fp, stderr=err_fp)

    return PNG_URL


## Remove temporary PNG images file
def rmTmpPngImages():

    global config  # noqa: F824
    function_name = "rmTmpPngImages:  "

    tmp_path = config.get("Applications", "TmpDir")
    ## Checking if path is found
    if not os.path.isdir(tmp_path):
        print("Content-type: text/html")
        print("")
        print(
            "<html><pre>"
            + function_name
            + ":The logging file: "
            + tmp_path
            + " does not exist; check the configuration file</pre></html>"
        )
        sys.exit(
            function_name
            + ":The logging file:  "
            + tmp_path
            + " does not exist.  Check the configuration file."
        )

    for file in os.listdir(tmp_path):
        filename = tmp_path + file
        if string.find(filename, "msautotest") != -1:
            logging.debug(function_name + "filename to remove: " + filename)
            os.remove(filename)


if __name__ == "__main__":
    main()
