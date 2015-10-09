
Pebble.addEventListener('ready',
  function(e) {
    console.log('JavaScript app ready and running!');
  }
);

function send_response() {

  console.log(this.responseText);
  var response = JSON.parse(this.responseText);
  var text = response.data.translations[0].translatedText;

  var t2 = JSON.parse(text);
  if (t2[0]) {
    text = t2[0]
  } else if (t2.demande) {
    text = t2.demande
  }

  console.log(text);

  var success = (this.status == 200);
  var dict = {
    response_success: success,
    response_text: success ? text : ""
  };

  Pebble.sendAppMessage(dict,
    function(e) {
      console.log('Send successful.');
    },
    function(e) {
      console.log('Send failed!');
    }
  );
}

Pebble.addEventListener('appmessage',
  function(e) {
    var text = JSON.stringify(e.payload)
    console.log('Received message: ' + text);

    var request = new XMLHttpRequest();
    request.addEventListener("load", send_response);
    var uri = "https://www.googleapis.com/language/translate/v2?key=YOUR_API_KEY_HERE&source=en" +
              "&target=fr&format=text&q=" + encodeURIComponent(text);
    console.log(uri);
    request.open("GET", uri);
    request.send();
  });
