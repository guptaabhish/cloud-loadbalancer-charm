/**
 * \addtogroup CkLdb
*/
/*@{*/

/** This code is derived from RefineLB.C, and RefineLB.C should
 be rewritten to use this, so there is no code duplication
*/

#include "Refiner.h"

int* Refiner::AllocProcs(int count, BaseLB::LDStats* stats)
{
  return new int[stats->n_objs];
}

void Refiner::FreeProcs(int* bufs)
{
  delete [] bufs;
}

void Refiner::create(int count, BaseLB::LDStats* stats, int* procs)
{
  int i;

  // now numComputes is all the computes: both migratable and nonmigratable.
  // afterwards, nonmigratable computes will be taken off

  unsigned int  maxspeed = 0;
    sumFreqs=0;
  numAvail = 0;
  for(i=0; i < P; i++) {
    processors[i].Id = i;
    processors[i].backgroundLoad = stats->procs[i].bg_walltime;
    processors[i].load = processors[i].backgroundLoad;
    processors[i].computeLoad = 0;
    processors[i].computeSet = new Set();
    processors[i].pe_speed = stats->procs[i].pe_speed;
    if (processors[i].pe_speed > maxspeed) maxspeed = processors[i].pe_speed;
    CmiPrintf(" PE SPEED %d %d \n", i, processors[i].pe_speed);
//    processors[i].utilization = stats->procs[i].utilization;
    processors[i].available = stats->procs[i].available;
    if (processors[i].available == true) numAvail++;
  }
 // Normalize PE SPeed

  for(int i=0; i < P; i++){
    processors[i].pe_speed *=100;
    processors[i].pe_speed /= maxspeed;
    sumFreqs+=processors[i].pe_speed;
    CmiPrintf(" PE SPEED %d %d \n", i, processors[i].pe_speed);
}


    CmiPrintf(" PE SPEED SUM %d %d \n", i, sumFreqs);

  for (i=0; i<stats->n_objs; i++)
  {
	LDObjData &odata = stats->objData[i];
	computes[i].Id = i;
        computes[i].id = odata.objID();
//        computes[i].handle = odata.handle;
        computes[i].load = odata.wallTime;     // was cpuTime
        computes[i].processor = -1;
        computes[i].oldProcessor = procs[i];
        computes[i].migratable = odata.migratable;
        if (computes[i].oldProcessor >= P)  {
 	  if (stats->complete_flag)
            CmiAbort("LB Panic: the old processor in RefineLB cannot be found, is this in a simulation mode?");
          else {
              // an object from outside domain, randomize its location
            computes[i].oldProcessor = CrnRand()%P;
	  }
	}
  }
//  for (i=0; i < numComputes; i++)
//      processors[computes[i].oldProcessor].computeLoad += computes[i].load;
}

void Refiner::assign(computeInfo *c, int processor)
{
  assign(c, &(processors[processor]));
}

void Refiner::assign(computeInfo *c, processorInfo *p)
{
   c->processor = p->Id;
   p->computeSet->insert((InfoRecord *) c);
   int oldPe=c->oldProcessor;
   p->computeLoad += c->load*p->pe_speed;
   p->load = p->computeLoad + p->backgroundLoad*p->pe_speed;

 //  p->computeLoad += c->load;
 //  p->load = p->computeLoad + p->backgroundLoad;
}

void  Refiner::deAssign(computeInfo *c, processorInfo *p)
{
   c->processor = -1;
   p->computeSet->remove(c);
   int oldPe=c->oldProcessor;
   p->computeLoad -= c->load*p->pe_speed;
   p->load = p->computeLoad + p->backgroundLoad*p->pe_speed;
//   p->computeLoad -= c->load;
  // p->load = p->computeLoad + p->backgroundLoad;
}




double Refiner::computeAverageLoad() {
  computeAverage();
  return averageLoad;
}

void Refiner::computeAverage()
{
  int i;
  double total = 0.;
  for (i=0; i<numComputes; i++) total += computes[i].load*processors[computes[i].oldProcessor].pe_speed;

  for (i=0; i<P; i++)
    if (processors[i].available == true) 
	total +=processors[i].backgroundLoad*processors[i].pe_speed;

  averageLoad = total/numAvail;
   totalInst=total;
}

double Refiner::computeMax()
{
  int i;
  double max = -1.0;
  for (i=0; i<P; i++) {
    if (processors[i].available == true && processors[i].load > max)
  //    max = processors[i].load;
      max=processors[i].load/processors[i].pe_speed;
  }
  return max;
}

double Refiner::computeMax(int *maxPe)
{
  int i;
  double max = -1.0,maxratio=-1.0;
  for (i=0; i<P; i++) {
//CkPrintf(" ********** pe%d load=%f freq=%d ratio=%f\n",processors[i].Id,processors[i].load,procFreqNew[processors[i].Id],processors[i].load/procFreqNew[processors[i].Id]);
    if (processors[i].available == CmiTrue && processors[i].load/processors[i].pe_speed > maxratio)
    {
//      max = processors[i].load;
//CkPrintf(" ********** pe%d load=%f freq=%d \n",processors[i].Id,processors[i].load,procFreqNew[processors[i].Id]);
        maxratio=processors[i].load/processors[i].pe_speed;
        max=processors[i].load;
        *maxPe=processors[i].Id;
    }
  }
  return max;
}

int Refiner::isHeavy(processorInfo *p)
{
  if (p->available == true) 
    // return p->load > overLoad*averageLoad;
    return p->load > overLoad*(totalInst*p->pe_speed/sumFreqs);
  else {
     return p->computeSet->numElements() != 0;
  }
}

int Refiner::isLight(processorInfo *p)
{
  if (p->available == true) 
//     return p->load < averageLoad;
        return p->load < totalInst*p->pe_speed;
  else 
     return 0;
}

// move the compute jobs out from unavailable PE
void Refiner::removeComputes()
{
  int first;
  Iterator nextCompute;

  if (numAvail < P) {
    if (numAvail == 0) CmiAbort("No processor available!");
    for (first=0; first<P; first++)
      if (processors[first].available == true) break;
    for (int i=0; i<P; i++) {
      if (processors[i].available == false) {
          computeInfo *c = (computeInfo *)
	           processors[i].computeSet->iterator((Iterator *)&nextCompute);
	  while (c) {
	    deAssign(c, &processors[i]);
	    assign(c, &processors[first]);
	    nextCompute.id++;
            c = (computeInfo *)
	           processors[i].computeSet->next((Iterator *)&nextCompute);
	  }
      }
    }
  }
}

int Refiner::refine()
{
  int i;
  int finish = 1;
  maxHeap *heavyProcessors = new maxHeap(P);

  Set *lightProcessors = new Set();
  for (i=0; i<P; i++) {
    if (isHeavy(&processors[i])) {  
      //      CkPrintf("Processor %d is HEAVY: load:%f averageLoad:%f!\n",
      //	       i, processors[i].load, averageLoad);
      heavyProcessors->insert((InfoRecord *) &(processors[i]));
    } else if (isLight(&processors[i])) {
      //      CkPrintf("Processor %d is LIGHT: load:%f averageLoad:%f!\n",
      //	       i, processors[i].load, averageLoad);
      lightProcessors->insert((InfoRecord *) &(processors[i]));
    }
  }
  int done = 0;

  while (!done) {
    double bestSize,bestIdle;
    computeInfo *bestCompute;
    processorInfo *bestP;
    
    processorInfo *donor = (processorInfo *) heavyProcessors->deleteMax();
    if (!donor) break;

    //find the best pair (c,receiver)
    Iterator nextProcessor;
    processorInfo *p = (processorInfo *) 
      lightProcessors->iterator((Iterator *) &nextProcessor);
    bestSize = 0;
    bestIdle = 0; 
    bestP = 0;
    bestCompute = 0;

    while (p) {
      Iterator nextCompute;
      nextCompute.id = 0;
      computeInfo *c = (computeInfo *) 
	donor->computeSet->iterator((Iterator *)&nextCompute);
      // iout << iINFO << "Considering Procsessor : " 
      //      << p->Id << "\n" << endi;
      while (c) {
        if (!c->migratable) {
	  nextCompute.id++;
	  c = (computeInfo *) 
	    donor->computeSet->next((Iterator *)&nextCompute);
          continue;
        }
//	CkPrintf("c->load: %f p->load:%f overLoad*averageLoad:%f \n",
//	c->load, p->load, overLoad*averageLoad);
//	CkPrintf("c->load * procpe_speed %f p->load:%f overLoad times%f Extra %f %f %f \n",  c->load*processors[c->oldProcessor].pe_speed , p->load ,overLoad*(totalInst*p->pe_speed/sumFreqs), averageLoad, p->load, bestIdle) ;
//	if ( c->load + p->load < overLoad*averageLoad && averageLoad-p->load>bestIdle) {
// TODO GET later
//	if ( c->load*processors[c->oldProcessor].pe_speed + p->load < overLoad*(totalInst*p->pe_speed/sumFreqs) && averageLoad-p->load>bestIdle) {
	if ( c->load*processors[c->oldProcessor].pe_speed + p->load < overLoad*(totalInst*p->pe_speed/sumFreqs) ) {
	// iout << iINFO << "Considering Compute : " 
	  //    << c->Id << " with load " 
	    //  << c->load << "\n" << endi;
//	CkPrintf("Inside c->load * procpe_speed %f p->load:%f overLoad times%f Extra %f %f %f \n",  c->load*processors[c->oldProcessor].pe_speed , p->load ,overLoad*(totalInst*p->pe_speed/sumFreqs), averageLoad, p->load, bestIdle) ;
	  if(c->load*processors[c->oldProcessor].pe_speed > bestSize) {
		bestIdle = averageLoad-p->load;
//	    bestSize = c->load;
	    bestSize = c->load*processors[c->oldProcessor].pe_speed;
	    bestCompute = c;
	    bestP = p;
	  }
	}
	nextCompute.id++;
	c = (computeInfo *) 
	  donor->computeSet->next((Iterator *)&nextCompute);
      }
      p = (processorInfo *) 
	lightProcessors->next((Iterator *) &nextProcessor);
    }

    if (bestCompute) {
CkPrintf("best load:%f\n",bestCompute->load);
      //      CkPrintf("Assign: [%d] with load: %f from %d to %d \n",
      //	       bestCompute->id.id[0], bestCompute->load, 
      //	       donor->Id, bestP->Id);
      deAssign(bestCompute, donor);      
      assign(bestCompute, bestP);
    } else {
      finish = 0;
      break;
    }

 //  if (bestP->load > averageLoad)
   if (bestP->load >  totalInst*bestP->pe_speed/sumFreqs )
      lightProcessors->remove(bestP);
    
    if (isHeavy(donor))
      heavyProcessors->insert((InfoRecord *) donor);
    else if (isLight(donor))
      lightProcessors->insert((InfoRecord *) donor);
  }  

  delete heavyProcessors;
  delete lightProcessors;

  return finish;
}

int Refiner::multirefine()
{
  computeAverage();
  double avg = averageLoad;
	  int maxPe=-1;
	//  double max = computeMax();
	  double max = computeMax(&maxPe);

  const double overloadStep = 0.01;
  const double overloadStart = 1.001;
//  double dCurOverload = max / avg;
	  double dCurOverload = max /(totalInst*processors[maxPe].pe_speed/sumFreqs); 

  int minOverload = 0;
  int maxOverload = (int)((dCurOverload - overloadStart)/overloadStep + 1);
  double dMinOverload = minOverload * overloadStep + overloadStart;
  double dMaxOverload = maxOverload * overloadStep + overloadStart;
  int curOverload;
  int refineDone = 0;
	 CmiPrintf("maxPe=%d max=%f myAvg=%f dMinOverload: %f dMaxOverload: %f\n",maxPe,max,(totalInst*processors[maxPe].pe_speed/sumFreqs), dMinOverload, dMaxOverload);
  if (_lb_args.debug()>=1)
    CmiPrintf("dMinOverload: %f dMaxOverload: %f\n", dMinOverload, dMaxOverload);
                                                                                
  overLoad = dMinOverload;
  if (refine())
    refineDone = 1;
  else {
    overLoad = dMaxOverload;
    if (!refine()) {
      CmiPrintf("ERROR: Could not refine at max overload\n");
      refineDone = 1;
    }
  }
                                                                                
  // Scan up, until we find a refine that works
  while (!refineDone) {
    if (maxOverload - minOverload <= 1)
      refineDone = 1;
    else {
      curOverload = (maxOverload + minOverload ) / 2;
                                                                                
      overLoad = curOverload * overloadStep + overloadStart;
      if (_lb_args.debug()>=1)
      CmiPrintf("Testing curOverload %d = %f [min,max]= %d, %d\n", curOverload, overLoad, minOverload, maxOverload);
      if (refine())
        maxOverload = curOverload;
      else
        minOverload = curOverload;
    }
  }
  return 1;
}

void Refiner::Refine(int count, BaseLB::LDStats* stats, 
		     int* cur_p, int* new_p,double lbTime,double *idleTime)
{
  //  CkPrintf("[%d] Refiner strategy\n",CkMyPe());

  P = count;
  numComputes = stats->n_objs;
  computes = new computeInfo[numComputes];
  processors = new processorInfo[count];

  create(count, stats, cur_p);
/////////////////////////////////
// cs598 osman

  double *tempLoad = new double[count];
  for(int p=0;p<count;p++) tempLoad[p]=0.0;
  for (int i=0; i<numComputes; i++)
  {
        CkPrintf("(%d:%d:%f), ",i, computes[i].oldProcessor, computes[i].load);
	tempLoad[computes[i].oldProcessor]+=computes[i].load;
  }
    for (int i=0; i<count; i++) CkPrintf(" Total Obj Loads%d:%f ", i, tempLoad[i]);

  for(int p=0;p<count;p++) 
  {
	processors[p].backgroundLoad = lbTime - tempLoad[p]-idleTime[p];
//if(p==0) processors[p].backgroundLoad = averageLoad*2;

/** Commented since happening later ***/
// 	processors[p].load +=processors[p].backgroundLoad;
  }

/////////////////////////////////


  int i;
  for (i=0; i<numComputes; i++)
    assign((computeInfo *) &(computes[i]),
           (processorInfo *) &(processors[computes[i].oldProcessor]));

  removeComputes();

  computeAverage();

//  if (_lb_args.debug()>2)  {
    CkPrintf("Old PE load (bg load): ");
    for (i=0; i<count; i++) CkPrintf("%d:%f(%f) ", i, processors[i].load, processors[i].backgroundLoad);
    CkPrintf("\n");
 // }

  multirefine();

  int nmoves = 0;
  for (int pe=0; pe < P; pe++) {
    Iterator nextCompute;
    nextCompute.id = 0;
    computeInfo *c = (computeInfo *)
      processors[pe].computeSet->iterator((Iterator *)&nextCompute);
    while(c) {
      new_p[c->Id] = c->processor;
      if (new_p[c->Id] != cur_p[c->Id]) nmoves++;
//      if (c->oldProcessor != c->processor)
//      CkPrintf("Refiner::Refine: from %d to %d\n", c->oldProcessor, c->processor);
      nextCompute.id++;
      c = (computeInfo *) processors[pe].computeSet->
	             next((Iterator *)&nextCompute);
    }
  }
  if (_lb_args.debug()>2)  {
    CkPrintf("New PE load: ");
    for (i=0; i<count; i++) CkPrintf("%f ", processors[i].load);
    CkPrintf("\n");
  }
  if (_lb_args.debug()>1) 
    CkPrintf("Refiner: moving %d obejcts. \n", nmoves);
  delete [] computes;
  delete [] processors;
}


/*@}*/
