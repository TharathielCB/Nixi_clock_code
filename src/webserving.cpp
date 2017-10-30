#include "webserving.h"

String html_header() {
    String header = "";
    header = "<head>";
    header += "<link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/siimple/2.0.1/siimple.min.css\" />";
    header += "<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat:400,700\" />";
    header += "</head>";
    header +="<style>";
    header +="body { padding: 0px; margin: 0px; color: #57607c; }";  
    header +=".website-header {display: block; width: 100%; padding-top: 0px; padding-bottom: 60px; margin-top: 0px; margin-bottom: 0px; background-color: #4894f0;  color: #ffffff; }";
    header +=".website-header-title {font-size: 55px; font-weight: bold; margin-bottom: 5px; }";                                                                                                                                                                                                   
    header +=".website-header-detail {font-size: 22px; font-weight: normal; }";
    header +=".website-main {display: block; padding-top: 30px; margin-left: auto; margin-right: auto; width: 95%; padding-bottom: 30px;}";
    header +="</style>";                                                                                                                                                                                                    

    return header;
}

String config_body() {
    String body = "";
    body += "<body>";                                                                                                                                                                                                      
    body += "<div class=\"website-header\">";                                                                                                                                                                           
    body += "<div class=\"website-header-title\">Nixie-Clock</div>";                                                                                                                                                
    body += "<div class=\"website-header-detail\">Device configuration</div>";                                                                                                                                        
    body += "</div>";                                                                                                                                                                                                  
    body += "<div class=\"website-main\">";                                                                                                                                                                             
    body += "<form  method=\"POST\" action=\"save\">";                                                                                                                                                                   
    body += "<p class=\"siimple-p\">";                                                                                                                                                                             
    body += "<div class=\"siimple-grid\">";                                                                                                                                                                           
    body += "<div class=\"siimple-grid-row\">";                                                                                                                                                                     
    body += "<div class=\"siimple-grid-col siimple-grid-col--2\"><label class=\"siimple-label\">SSID:</label></div>";                                                                                               
    body += "<div class=\"siimple-grid-col\"><input type=\"text\" name=\"ssid\" class=\"siimple-input\" placeholder=\"YOUR-NETWORK\"></div>";                                                                             
    body += "</div>";                                                                                                                                                                                            
    body += "<div class=\"siimple-grid-row\">";                                                                                                                                                                     
    body += "<div class=\"siimple-grid-col siimple-grid-col--2\"><label class=\"siimple-label\">Wifi Password:</label></div>";                                                                                      
    body += "<div class=\"siimple-grid-col\"><input type=\"password\" name=\"password\" class=\"siimple-input\" placeholder=\"SUPERSECRET\"></div>";
    body += "</div>";
    body += "</p>";                                                                                                                                                                                               
    body += "<p class=\"siimple-p\">";                                                                                                                                                                              
    body += "<div class=\"siimple-grid-row\">";                                                                                                                                                                     
    body += "<div class=\"siimple-grid-col siimple-grid-col--2\"><label class=\"siimple-label\">NTP-Server:</label></div>";
    body += "<div class=\"siimple-grid-col\"><input type=\"text\" name=\"ntp\" class=\"siimple-input\" placeholder=\"TIMESERVER\"></div>";
    body += "</div>";
    body += "<div class=\"siimple-grid-row\">";
    body += "<div class=\"siimple-grid-col siimple-grid-col--2\"><label class=\"siimple-label\">MQTT-Broker:</label></div>";
    body += "<div class=\"siimple-grid-col\"><input type=\"text\" name=\"mqtt\"  class=\"siimple-input\" placeholder=\"MQTT-Broker\"></div>";
    body += "</div>";
    body += "</div>";
    body += "</p>";
    body += "<input class=\"siimple-btn siimple-btn--blue\" type=\"submit\" value=\"save settings\">";
    body += "</form>";
    body += "</div>";                                                                                                                                                                                                  
    body += "</body>";

    return body;
}


String config_form() {
    String s = "<html>";
    s += html_header();
    s += config_body();
    s += "</html>\r\n\r\n";

    return s;
}