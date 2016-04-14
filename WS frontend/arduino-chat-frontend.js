$(function () {
    "use strict";

    // for better performance - to avoid searching in DOM
    var content = $('#content');
    var input = $('#input');
    var status = $('#status');

    // my color assigned by the server
    var myColor = true;
    // my name sent to the server
    var myName = true;
	
    // if user is running mozilla then use it's built-in WebSocket
    window.WebSocket = window.WebSocket || window.MozWebSocket;
	
    // if browser doesn't support WebSocket, just show some notification and exit
    if (!window.WebSocket) {
        content.html($('<p>', { text: 'Sorry, but your browser doesn\'t '
                                    + 'support WebSockets.'} ));
        input.hide();
        $('span').hide();
        return;
    }

    // open connection--overwrite default string for your value.
	var chariot = prompt("Please enter Chariot WS server: ", "ws://192.168.0.77:1337");
	var connection = new WebSocket(chariot);
 
    connection.onopen = function () {
        // first we want users to enter their names
        input.removeAttr('disabled');
        status.text('Enter cmd/URL ');
    };

    connection.onerror = function (error) {
        // just in case there were some problems with connection...
        content.html($('<p>', { text: 'Sorry, but there\'s some problem with your '
                                    + 'connection or the server is down.' } ));
    };

    // most important part - incoming messages
    connection.onmessage = function (event) {
		input.removeAttr('disabled'); // let the user write another message
		//content.prepend('<p> ' +
        //     + 'Arduino IoT: ' + event.data + '</p>');
		content.prepend('<p> ' + event.data + '</p>');
		//addMessage();
    };

    /**
     * Send mesage when user presses Enter key
     */
    input.keydown(function(e) {
        if (e.keyCode === 13) {
            var msg = $(this).val();
            if (!msg) {
                return;
            }
            // send the message as an ordinary text
			content.prepend('<p> ' + msg + '</p>');
            connection.send(msg);
            $(this).val('');
            // disable the input field to make the user wait until server
            // sends back response
            //input.attr('disabled', 'disabled');

            // we know that the first message sent from a user their name
            //if (myName === false) {
            //    myName = msg;
            //}
        }
    });

    /**
     * This method is optional. If the server wasn't able to respond to the
     * in 30 seconds then show some error message to notify the user that
     * something is wrong. We're talking to an Arduino WoT!
     */
    setInterval(function() {
        if (connection.readyState !== 1) {
            status.text('Error');
            input.attr('disabled', 'disabled').val('Unable to communicate '
                                                 + 'with the WebSocket server.');
        }
    }, 60000);

    /**
     * Add message to the chat window
     */
    function addMessage() {
        content.prepend('<p> ' +
             + 'Arduino IoT: ' + message.data + '</p>');
    }
});