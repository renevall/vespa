// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.jdisc.http.server.jetty;

import com.yahoo.jdisc.http.server.jetty.HttpResponseStatisticsCollector.StatisticsEntry;
import org.eclipse.jetty.http.HttpFields;
import org.eclipse.jetty.http.HttpURI;
import org.eclipse.jetty.http.HttpVersion;
import org.eclipse.jetty.http.MetaData;
import org.eclipse.jetty.http.MetaData.Response;
import org.eclipse.jetty.server.AbstractConnector;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.HttpChannel;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.HttpTransport;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;
import org.eclipse.jetty.util.Callback;
import org.junit.Before;
import org.junit.Test;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

/**
 * @author ollivir
 */
public class HttpResponseStatisticsCollectorTest {

    private Connector connector;
    private List<String> monitoringPaths = List.of("/status.html");
    private List<String> searchPaths = List.of("/search");
    private HttpResponseStatisticsCollector collector = new HttpResponseStatisticsCollector(monitoringPaths, searchPaths);
    private int httpResponseCode = 500;

    @Test
    public void statistics_are_aggregated_by_category() {
        testRequest("http", 300, "GET");
        testRequest("http", 301, "GET");
        testRequest("http", 200, "GET");

        var stats = collector.takeStatistics();
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, 1L);
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_3XX, 2L);
    }

    @Test
    public void statistics_are_grouped_by_http_method_and_scheme() {
        testRequest("http", 200, "GET");
        testRequest("http", 200, "PUT");
        testRequest("http", 200, "POST");
        testRequest("http", 200, "POST");
        testRequest("http", 404, "GET");
        testRequest("https", 404, "GET");
        testRequest("https", 200, "POST");
        testRequest("https", 200, "POST");
        testRequest("https", 200, "POST");
        testRequest("https", 200, "POST");

        var stats = collector.takeStatistics();
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, 1L);
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_4XX, 1L);
        assertStatisticsEntryPresent(stats, "http", "PUT", MetricDefinitions.RESPONSES_2XX, 1L);
        assertStatisticsEntryPresent(stats, "http", "POST", MetricDefinitions.RESPONSES_2XX, 2L);
        assertStatisticsEntryPresent(stats, "https", "GET", MetricDefinitions.RESPONSES_4XX, 1L);
        assertStatisticsEntryPresent(stats, "https", "POST", MetricDefinitions.RESPONSES_2XX, 4L);
    }

    @Test
    public void statistics_include_grouped_and_single_statuscodes() {
        testRequest("http", 401, "GET");
        testRequest("http", 404, "GET");
        testRequest("http", 403, "GET");

        var stats = collector.takeStatistics();
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_4XX, 3L);
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_401, 1L);
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_403, 1L);

    }

    @Test
    public void retrieving_statistics_resets_the_counters() {
        testRequest("http", 200, "GET");
        testRequest("http", 200, "GET");

        var stats = collector.takeStatistics();
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, 2L);

        testRequest("http", 200, "GET");

        stats = collector.takeStatistics();
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, 1L);
    }

    @Test
    public void statistics_include_request_type_dimension() {
        testRequest("http", 200, "GET", "/search");
        testRequest("http", 200, "POST", "/search");
        testRequest("http", 200, "POST", "/feed");
        testRequest("http", 200, "GET", "/status.html?foo=bar");

        var stats = collector.takeStatistics();
        assertStatisticsEntryWithRequestTypePresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, "monitoring", 1L);
        assertStatisticsEntryWithRequestTypePresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, "read", 1L);
        assertStatisticsEntryWithRequestTypePresent(stats, "http", "POST", MetricDefinitions.RESPONSES_2XX, "read", 1L);
        assertStatisticsEntryWithRequestTypePresent(stats, "http", "POST", MetricDefinitions.RESPONSES_2XX, "write", 1L);

        testRequest("http", 200, "GET");

        stats = collector.takeStatistics();
        assertStatisticsEntryPresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, 1L);
    }

    @Test
    public void request_type_can_be_set_explicitly() {
        testRequest("http", 200, "GET", "/search", com.yahoo.jdisc.Request.RequestType.WRITE);

        var stats = collector.takeStatistics();
        assertStatisticsEntryWithRequestTypePresent(stats, "http", "GET", MetricDefinitions.RESPONSES_2XX, "write", 1L);
    }

    @Before
    public void initializeCollector() throws Exception {
        Server server = new Server();
        connector = new AbstractConnector(server, null, null, null, 0) {
            @Override
            protected void accept(int acceptorID) throws IOException, InterruptedException {
            }

            @Override
            public Object getTransport() {
                return null;
            }
        };
        collector.setHandler(new AbstractHandler() {
            @Override
            public void handle(String target, Request baseRequest, HttpServletRequest request, HttpServletResponse response)
                    throws IOException, ServletException {
                baseRequest.setHandled(true);
                baseRequest.getResponse().setStatus(httpResponseCode);
            }
        });
        server.setHandler(collector);
        server.start();
    }

    private Request testRequest(String scheme, int responseCode, String httpMethod) {
        return testRequest(scheme, responseCode, httpMethod, "foo/bar");
    }
    private Request testRequest(String scheme, int responseCode, String httpMethod, String path) {
        return testRequest(scheme, responseCode, httpMethod, path, null);
    }
    private Request testRequest(String scheme, int responseCode, String httpMethod, String path,
                                com.yahoo.jdisc.Request.RequestType explicitRequestType) {
        HttpChannel channel = new HttpChannel(connector, new HttpConfiguration(), null, new DummyTransport());
        MetaData.Request metaData = new MetaData.Request(httpMethod, new HttpURI(scheme + "://" + path), HttpVersion.HTTP_1_1, new HttpFields());
        Request req = channel.getRequest();
        if (explicitRequestType != null)
            req.setAttribute("requestType", explicitRequestType);
        req.setMetaData(metaData);

        this.httpResponseCode = responseCode;
        channel.handle();
        return req;
    }

    private static void assertStatisticsEntryPresent(List<StatisticsEntry> result, String scheme, String method, String name, long expectedValue) {
        long value = result.stream()
                .filter(entry -> entry.method.equals(method) && entry.scheme.equals(scheme) && entry.name.equals(name))
                .mapToLong(entry -> entry.value)
                .findAny()
                .orElseThrow(() -> new AssertionError(String.format("Not matching entry in result (scheme=%s, method=%s, name=%s)", scheme, method, name)));
        assertThat(value, equalTo(expectedValue));
    }

    private static void assertStatisticsEntryWithRequestTypePresent(List<StatisticsEntry> result, String scheme, String method, String name, String requestType, long expectedValue) {
        long value = result.stream()
                .filter(entry -> entry.method.equals(method) && entry.scheme.equals(scheme) && entry.name.equals(name) && entry.requestType.equals(requestType))
                .mapToLong(entry -> entry.value)
                .reduce(Long::sum)
                .orElseThrow(() -> new AssertionError(String.format("Not matching entry in result (scheme=%s, method=%s, name=%s, type=%s)", scheme, method, name, requestType)));
        assertThat(value, equalTo(expectedValue));
    }

    private final class DummyTransport implements HttpTransport {
        @Override
        public void send(Response info, boolean head, ByteBuffer content, boolean lastContent, Callback callback) {
            callback.succeeded();
        }

        @Override
        public boolean isPushSupported() {
            return false;
        }

        @Override
        public boolean isOptimizedForDirectBuffers() {
            return false;
        }

        @Override
        public void push(MetaData.Request request) {
        }

        @Override
        public void onCompleted() {
        }

        @Override
        public void abort(Throwable failure) {
        }
    }
}
