// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.processing.execution.test;

import com.yahoo.component.chain.Chain;
import com.yahoo.processing.Processor;
import com.yahoo.processing.Request;
import com.yahoo.processing.Response;
import com.yahoo.processing.execution.Execution;
import com.yahoo.processing.response.DataList;
import org.junit.Test;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import static com.yahoo.processing.test.ProcessorLibrary.*;
import static org.junit.Assert.assertEquals;

/**
 * Tests scenarios where a data producer returns a promise of some future data rather than the data itself.
 * As no processor waits for the data it is returned all the way to the caller.
 *
 * @author  bratseth
 */
public class FutureDataTestCase {

    /** Run a chain which ends in a processor which returns a response containing future data. */
    @SuppressWarnings("unchecked")
    @Test
    public void testFutureDataPassThrough() throws InterruptedException, ExecutionException, TimeoutException {
        // Set up
        FutureDataSource futureDataSource=new FutureDataSource();
        Chain<Processor> chain=new Chain<>(new DataCounter(),futureDataSource);

        // Execute
        Request request=new Request();
        Response response=Execution.createRoot(chain,0,Execution.Environment.createEmpty()).process(request); // Urk ...

        // Verify the result prior to completion of delayed data
        assertEquals(1,response.data().asList().size());
        assertEquals("Data count: 0",response.data().get(0).toString());

        // complete delayed data
        assertEquals("Delayed data was requested once", 1, futureDataSource.incomingData.size());
        futureDataSource.incomingData.get(0).add(new StringData(request, "d1"));
        futureDataSource.incomingData.get(0).addLast(new StringData(request, "d2"));
        assertEquals("New data is not visible because we haven't asked for it", 1, response.data().asList().size());
        response.data().complete().get(1000, TimeUnit.MILLISECONDS);
        assertEquals("Now the data is available", 3, response.data().asList().size());
        assertEquals("d1",response.data().get(1).toString().toString());
        assertEquals("d2",response.data().get(2).toString().toString());
    }

    /** Federate to one source which returns data immediately and one who return future data */
    @SuppressWarnings("unchecked")
    @Test
    public void testFederateSyncAndAsyncData() throws InterruptedException, ExecutionException, TimeoutException {
        // Set up
        FutureDataSource futureDataSource=new FutureDataSource();
        Chain<Processor> chain=new Chain<>(new DataCounter(),new Federator(new Chain<>(new DataSource()),new Chain<>(futureDataSource)));

        // Execute
        Request request=new Request();
        request.properties().set("appendage",1);
        Response response=Execution.createRoot(chain,0,Execution.Environment.createEmpty()).process(request);

        // Verify the result prior to completion of delayed data
        assertEquals(3,response.data().asList().size()); // The sync data list + the (currently empty) future data list) + the data count
        DataList syncData=(DataList)response.data().get(0);
        DataList asyncData=(DataList)response.data().get(1);
        StringData countData=(StringData)response.data().get(2);

        assertEquals("The sync data is available",3,syncData.asList().size());
        assertEquals( "first.1",syncData.get(0).toString());
        assertEquals("second.1", syncData.get(1).toString());
        assertEquals( "third.1",syncData.get(2).toString());
        assertEquals("No async data yet",0,asyncData.asList().size());
        assertEquals("The data counter has run and accessed the sync data","Data count: 3",countData.toString());

        // complete async data
        futureDataSource.incomingData.get(0).add(new StringData(request, "d1"));
        futureDataSource.incomingData.get(0).addLast(new StringData(request, "d2"));
        assertEquals("New data is not visible because we haven't asked for it", 0, asyncData.asList().size());
        asyncData.complete().get(1000, TimeUnit.MILLISECONDS);
        assertEquals("Now the data is available", 2, asyncData.asList().size());
        assertEquals("d1",asyncData.get(0).toString().toString());
        assertEquals("d2", asyncData.get(1).toString().toString());
    }

    /** Register a chain which will be called when some async data is available */
    @SuppressWarnings("unchecked")
    @Test
    public void testAsyncDataProcessing() throws InterruptedException, ExecutionException, TimeoutException {
        // Set up
        FutureDataSource futureDataSource=new FutureDataSource();
        Chain<Processor> asyncChain=new Chain<Processor>(new DataCounter());
        Chain<Processor> chain=new Chain<>(new AsyncDataProcessingInitiator(asyncChain),futureDataSource);

        // Execute
        Request request=new Request();
        Response response=Execution.createRoot(chain,0,Execution.Environment.createEmpty()).process(request);

        // Verify the result prior to completion of delayed data
        assertEquals("No data yet",0,response.data().asList().size());

        // complete async data
        futureDataSource.incomingData.get(0).add(new StringData(request, "d1"));
        assertEquals("New data is not visible because it is not complete", 0, response.data().asList().size());
        futureDataSource.incomingData.get(0).addLast(new StringData(request, "d2"));
        assertEquals("Not visible because it has not been synced yet", 0, response.data().asList().size());
        response.data().complete().get(1000, TimeUnit.MILLISECONDS);
        assertEquals("Now the data as well as the count is available", 3, response.data().asList().size());
        assertEquals("d1",response.data().get(0).toString().toString());
        assertEquals("d2",response.data().get(1).toString().toString());
        assertEquals("Data count: 2",response.data().get(2).toString());
    }

    /**
     * Register a chain which federates over three sources, two of which are future.
     * When the first of the futures are done one additional chain is to be run.
     * When both are done another chain is to be run.
     */
    @SuppressWarnings("unchecked")
    @Test
    public void testAsyncDataProcessingOfFederatedResult() throws InterruptedException, ExecutionException, TimeoutException {
        // Set up
        // Source 1 (async with completion chain)
        FutureDataSource futureSource1=new FutureDataSource();
        Chain<Processor> asyncChainSource1=new Chain<Processor>(new DataCounter("source1"));
        Chain<Processor> chainSource1=new Chain<>(new AsyncDataProcessingInitiator(asyncChainSource1),futureSource1);
        // Source 2 (async source)
        FutureDataSource futureSource2=new FutureDataSource();
        Chain<Processor> chainSource2=new Chain<Processor>(futureSource2);
        // Source 3 (sync source)
        Chain<Processor> chainSource3=new Chain<Processor>(new DataSource());
        // Main chain federating to the above - not waiting for source 1 and 2 but invoking asyncMain when both are complete
        Chain<Processor> asyncMain=new Chain<Processor>(new DataCounter("main"));
        Chain<Processor> main=new Chain<>(new AsyncDataProcessingInitiator(asyncMain),new Federator(chainSource1,chainSource2,chainSource3));

        // Execute
        Request request=new Request();
        Response response=Execution.createRoot(main,0,Execution.Environment.createEmpty()).process(request);

        // Verify the result prior to completion of delayed data
        assertEquals("We have the sync data plus placeholders for the async lists",3,response.data().asList().size());
        DataList source1Data=((DataList)response.data().get(0));
        DataList source2Data=((DataList)response.data().get(1));
        DataList source3Data=((DataList)response.data().get(2));

        assertEquals("No data yet",0,source1Data.asList().size());
        assertEquals("No data yet",0,source2Data.asList().size());
        assertEquals(3,source3Data.asList().size());

        // complete async data in source1
        futureSource1.incomingData.get(0).addLast(new StringData(request,"source1Data"));
        assertEquals("Not visible yet", 0, source1Data.asList().size());
        source1Data.complete().get(1000, TimeUnit.MILLISECONDS);
        assertEquals(2, source1Data.asList().size());
        assertEquals("source1Data",source1Data.get(0).toString());
        assertEquals("Completion listener chain on this has run", "[source1] Data count: 1", source1Data.get(1).toString());

        // source2 & main completion
        assertEquals("Main completion listener has not run", 3, response.data().asList().size());
        futureSource2.incomingData.get(0).addLast(new StringData(request, "source2Data"));
        assertEquals("Main completion listener has not run", 3, response.data().asList().size());

        Response.recursiveComplete(response.data()).get();
        assertEquals("Main completion listener has run", 4, response.data().asList().size());
        assertEquals("The main data counter saw all sync data, but not source2 data as it executes after this",
                     "[main] Data count: " + (2 + 0 + 3), response.data().get(3).toString());
    }

}
