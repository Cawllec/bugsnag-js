Feature: Delivery of errors

  Scenario: Delivery is attempted oversized payloads
    When I navigate to the test URL "/delivery/script/a.html"
    And I wait for 20 seconds
    And I set the HTTP status code to 500
    And I wait to receive an error
    
    # Check that Bugsnag is discarding the event
    And I wait to receive a log
    And the log payload field "level" equals "warning"
    Then the log payload field "message" matches the regex "Discarding over-sized event \(from.*\) after failed delivery"