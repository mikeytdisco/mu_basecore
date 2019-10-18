import os
import sys
import traceback

from flask import Flask, request, send_file, Response, jsonify

app = Flask(__name__)
#
# Max Content size is from:
#     #define MAX_ALLOWABLE_MS_IDENTITY_AUTH_PROVISION_APPLY_VAR_SIZE (1024 * 16)
#
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024
app.config['UPLOAD_FOLDER'] = '/Dfci_Database'

RqstTypes = ['Identity', 'Identity2', 'Permissions', 'Permissions2', 'Settings', 'Settings2', 'Current']
DATABASE_FOLDER = '/Dfci_Database'

#
#  Default web page for this server to verify that the test DFCI server is functional.
#
@app.route('/')
def hello_world():
    try:
        msg = 'Hello, World! DFCI Test Server V 2.0 serving BootShell and DfciRequest.'
        msg += '\r\rRequest from '
        msg += request.remote_addr
        return msg
    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r
#
#  A location to boot the x64 shell in a HTTP boot option
#
@app.route('/BootShell')
def boot_shell():

    try:
        ua = request.headers.get('User-Agent')

        if 'UefiHttpBoot' in ua:
            filename = 'Shell_Full.efi'
        else:
            filename = 'Shell.efi'

        if os.path.exists('static/' + filename) == False:
            msg = 'ServerError. Unable to find shell' + filename
            r = app.make_response(msg)
            r.mimetype = 'text/plain'
        else:
            r = app.make_response(app.send_static_file(filename))
            r.mimetype = 'application/efi'

        return r

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r

#
#  Early DFCI test web page for processing Dfci requests.
#
@app.route('/DfciRequest/<MachineId>/<RequestType>', methods=['GET', 'PUT'])
def Semm_Request(MachineId, RequestType):

    #  OLD style http test code.  This is left here as sample code, but it
    #  is not used by the current DfciRequest code.
    #
    # A Dfci request comes in the form, and may be an HTTP GET or HTTP PUT:
    #       <BaseUrl>/DfciRequest/<MachineId>/<RequestType>
    #
    # Request types are Identity, Permissions, and Settings.  That means there are
    # six expected requests:
    #
    #   1. PUT of Provision Results when RequestType == "Identity"
    #   2. PUT of Permissions Results when RequestType == "Permissions"
    #   3. PUT of Settings Results when RequestType == "Settings"
    #   4. PUT of Current Settings Xml RequestType == "Current"
    #   5. GET of Provision packet when RequestType == "Identity"
    #   6. GET of Permissions packet when RequestType == "Permissions"
    #   7. GET of Settings packet when RequestType == "Settings"
    #
    # All request are for a particular MachineId.  It would be up to the Web Server
    # implementation to determine if a system is in a particular group and handle group
    # settings.  In this test server, the new deployment packets and results data is stored
    # in a simple file database at \Dfci_Database\<MachineId>\
    #
    #

    try:
        ua = request.headers.get('User-Agent')

        if 'DFCI-Agent' in ua:
            # Use MachineId to locate a particular packet name from database

            method = request.method
            if RequestType not in RqstTypes:
                msg = 'DFCI Error. Invalid request type'
                r = app.make_response(msg)
                r.mimetype = 'text/plain'
                return r

            filelocation = os.path.join(app.config['UPLOAD_FOLDER'],MachineId)
            if not os.path.isdir (filelocation):
                msg = 'DFCI Error. Unknown system'
                r = app.make_response(msg)
                r.mimetype = 'text/plain'
                return r

            if method == 'PUT':
                filename = 'Dfci_Result_' + RequestType
                if (RequestType == 'Current'):
                    filename = filename + '.xml'
                else:
                    filename = filename + '.bin'
                filename = os.path.join(filelocation,filename)
                file = open(filename,'wb')
                file.write(request.data)
                file.close()
                r = app.make_response('Result uploaded')
                r.mimetype = 'text/plain'
                return r

            if (RequestType == 'Current'):
                msg = 'DFCI Error. Current cannot be requested'
                r = app.make_response(msg)
                r.mimetype = 'text/plain'
                return r

            filename = 'Dfci_Apply_' + RequestType + '.bin'
            filename = os.path.join(filelocation,filename)
            r = app.make_response(send_file(filename))
            r.headers["Cache-Control"] = "must-revalidate"
            r.headers["Pragma"] = "must-revalidate"
            r.mimetype = 'application/octet-stream'
            return r
        else:
            msg = ''.join(traceback.format_exc())
            r = app.make_response(msg)
            r.mimetype = 'text/plain'
            r.status_code = 503
            return r

    except Exception as e:
         msg = ''.join(traceback.format_exc())
         r = app.make_response(msg)
         r.mimetype = 'text/plain'
         r.status_code = 503
         return r

#
# Test recovery bootstrap InTune model.  This is an async request with the response
# telling Dfci where to go to get the payload/
#
@app.route('/ztd/noauth/dfci/recovery-bootstrap/', methods=['POST'])
def ZtdBootstrap_Request():

    #
    # A Ztd bootstrap request comes with a JSON Body:
    #
    #
    try:
        #
        # Expect a body, and store the body for later verification
        #
        if request.mimetype == 'application/json':
            filename = 'Bootstrap_Request.json'
            pathname = os.path.join(app.config['UPLOAD_FOLDER'],filename)
            file = open(pathname,'wb')
            file.write(request.data)
            file.close()

            response = jsonify()
            response.status_code = 202
            #
            location = '/ztd/unauth/dfci/recovery-bootstrap-status/{request-id}'
            response.headers['location'] = location
            return response
        else:
            response = jsonify()
            response.status_code = 406
            return response

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r

#
# Test recovery bootstrap response server.  This test the client to see if the certs
# need updating.  If so, cert update packets are returned If not, returns a "NULL" response.
#
@app.route('/ztd/unauth/dfci/recovery-bootstrap-status/{request-id}', methods=['GET'])
def ZtdBootstrap_Response():

    try:

        RequestName = os.path.join(app.config['UPLOAD_FOLDER'],'Bootstrap_Request.json')
        ExpectedName = os.path.join(app.config['UPLOAD_FOLDER'],'Expected_Request.json')

        try:
            with open(RequestName, "r") as RequestFile:
                RequestedData = RequestFile.read()
        except Exception as e:
            RequestedData = None

        try:
            with open(ExpectedName, "r") as ExpectedFile:
                ExpectedData = ExpectedFile.read()
        except Exception as e:
            ExpectedData = None

        filename = 'Bootstrap_Response.json'

        if RequestedData != None and RequestedData == ExpectedData:
            filename = 'Bootstrap_NULLResponse.json'

        pathname = os.path.join(app.config['UPLOAD_FOLDER'],filename)
        r = app.make_response(send_file(pathname))
        r.headers["Cache-Control"] = "must-revalidate"
        r.headers["Pragma"] = "must-revalidate"
        r.mimetype = 'application/json'
        r.status_code = 200
        return r

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r
#
# Update the device settings etc after the certs are updated.  This is another
# async request where a location header is specified to inform Dfci where to
# retrieve the update packets.
#
@app.route('/ztd/unauth/dfci/recovery-packets/', methods=['POST'])
def Recovery_Request():

    #
    # A Recovery request comes with a JSON Body:
    #
    #
    try:
        #
        # Expect a body, and store the body for later verification
        #
        if request.mimetype == 'application/json':
            filename = 'Recovery_Request.json'
            pathname = os.path.join(app.config['UPLOAD_FOLDER'],filename)
            file = open(pathname,'wb')
            file.write(request.data)
            file.close()

            response = jsonify()
            response.status_code = 202
            #
            # return a full URL with https instead of http
            response.autocorrect_location_header = False
            location = 'https://mikeytbds3.eastus.cloudapp.azure.com/ztd/unauth/dfci/recovery-packets-status/{request-id}'
            response.headers['location'] = location
            return response
        else:
            response = jsonify()
            response.status_code = 406
            return response

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r

#
# Recovery packet response location.  Return updated settings.
#
@app.route('/ztd/unauth/dfci/recovery-packets-status/{request-id}', methods=['GET'])
def Recovery_Response():
    try:
        filename = 'Recovery_Response.json'
        pathname = os.path.join(app.config['UPLOAD_FOLDER'],filename)
        r = app.make_response(send_file(pathname))
        r.headers["Cache-Control"] = "must-revalidate"
        r.headers["Pragma"] = "must-revalidate"
        r.mimetype = 'application/json'
        r.status_code = 200
        return r

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r

#
# Test model for HttpGet with redirect to an https location.
#
@app.route('/RedirTest1', methods=['GET'])
def RedirTest1_Redirect():
    #
    # A Recovery request comes with a JSON Body:
    #
    #
    try:
        #
        # No body expected
        #
        response = jsonify()
        response.status_code = 302
        #
        # return a full URL with https instead of http
        response.autocorrect_location_header = False
        location = 'https://mikeytbds3.eastus.cloudapp.azure.com/RedirTest2'
        response.headers['location'] = location
        return response

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r
#
# Test model for HttpGet Redirect target.
#
@app.route('/RedirTest2', methods=['GET'])
def RedirTest2_Response():
    #
    # A Recovery request comes with a JSON Body:
    #
    #
    try:
        #
        # No body expected
        #
        filename = 'RedirTest1_Response.json'
        pathname = os.path.join(app.config['UPLOAD_FOLDER'],filename)
        r = app.make_response(send_file(pathname))
        r.headers["Cache-Control"] = "must-revalidate"
        r.headers["Pragma"] = "must-revalidate"
        r.mimetype = 'application/json'
        r.status_code = 200
        return r

    except Exception as e:
        msg = ''.join(traceback.format_exc())
        r = app.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 503
        return r

if __name__ == '__main__':
    app.debug = true
    app.run(host='0.0.0.0', port=443)


