/**
 * Exporting data through url encoding
 */

// @see https://stackoverflow.com/questions/14964035/how-to-export-javascript-array-info-to-csv-on-client-side

function exportString(filename, str, type){
  if(!type)
    type = 'text/plain;charset=utf-8;';
  var blob = new Blob([str], { type: type });
  if(navigator.msSaveBlob) // IE 10+
    navigator.msSaveBlob(blob, filename);
  else {
    var url = URL.createObjectURL(blob);
    var link = document.createElement("a");
    if(link.download !== undefined){
      link.setAttribute('href', url);
      link.setAttribute('download', filename);
      link.style = "visibility:hidden;";
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    } else {
      window.open(url);
    }
  }
}

function exportJSON(filename, data, asStr) {
  if(arguments.length == 1){
    data = arguments[0];
    filename = 'result.json';
  }
  if(asStr)
    return 'data:text/json;charset=utf-8;' + JSON.stringify(data);
  else
    exportString(filename, JSON.stringify(data), 'text/json;charset=utf-8;');
}
