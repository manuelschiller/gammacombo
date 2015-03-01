#include "CLIntervalMaker.h"

CLIntervalMaker::CLIntervalMaker(OptParser *arg, const TH1F &pvalues)
: _pvalues(pvalues)
{
	assert(arg);
	_arg = arg;
	// for ( int i=1; i<pvalues.GetNbinsX(); i++ )
	// 	cout << i << " " << pvalues.GetBinCenter(i) << " " << pvalues.GetBinContent(i) << endl;
}

CLIntervalMaker::~CLIntervalMaker()
{}

///
/// Add a maximum (e.g. found by the Prob method) to calculate the
/// confidence intervals around it.
/// \param value - parameter value at the maximum
/// \param method - details on how this maximum was found
///
void CLIntervalMaker::provideMorePreciseMaximum(float value, TString method)
{
	// cout << "CLIntervalMaker::provideMorePreciseMaximum() : " << value << endl;
	float level = 1.; // accept this many bin sizes deviation
	for ( int i=0; i<_clintervals1sigma.size(); i++ ){
		if ( fabs(_clintervals1sigma[i].central - value) < level*_pvalues.GetBinWidth(1) ){
			_clintervals1sigma[i].central = value;
			_clintervals1sigma[i].centralmethod = method;
			_clintervals1sigma[i].pvalueAtCentral = _pvalues.GetBinContent(valueToBin(value));
		}
	}
	for ( int i=0; i<_clintervals2sigma.size(); i++ ){
		if ( fabs(_clintervals2sigma[i].central - value) < level*_pvalues.GetBinWidth(1) ){
			_clintervals2sigma[i].central = value;
			_clintervals2sigma[i].centralmethod = method;
			_clintervals2sigma[i].pvalueAtCentral = _pvalues.GetBinContent(valueToBin(value));
		}
	}
}

///
/// Finds the maxima of the _pvalue histogram and fills
/// _clintervals1sigma and _clintervals2sigma. Only accepts
/// maxima that are not similar to existing ones, that were, e.g.,
/// set by addMaximum().
/// \param pValueThreshold - ignore maxima under this pvalue threshold
/// 				to reject low statistics plugin crap
///
void CLIntervalMaker::findMaxima(float pValueThreshold)
{
	for ( int i=2; i<_pvalues.GetNbinsX()-1; i++ ){
		if ( _pvalues.GetBinContent(i-1) < _pvalues.GetBinContent(i)
			&& _pvalues.GetBinContent(i)   > _pvalues.GetBinContent(i+1) ){
			if ( _pvalues.GetBinContent(i) > pValueThreshold ){
				CLInterval cli;
				cli.centralmethod = "max bin";
				// cli.centralmethod = Form("maximum bin above p=%.2f",pValueThreshold);
				cli.central = _pvalues.GetBinCenter(i);
				cli.pvalueAtCentral = _pvalues.GetBinContent(i);
				_clintervals1sigma.push_back(cli); // push_back stores copies
				_clintervals2sigma.push_back(cli);
			}
		}
	}
}
// void CLIntervalMaker::findMaxima(float pValueThreshold)
// {
// 	for ( int i=3; i<_pvalues.GetNbinsX()-2; i++ ){
// 		if ( _pvalues.GetBinContent(i-1) < _pvalues.GetBinContent(i)
// 			&& _pvalues.GetBinContent(i)   > _pvalues.GetBinContent(i+1)
// 			&& _pvalues.GetBinContent(i-2) < _pvalues.GetBinContent(i-1)
// 			&& _pvalues.GetBinContent(i+1) > _pvalues.GetBinContent(i+2)
// 			){
// 			if ( _pvalues.GetBinContent(i) > pValueThreshold ){
// 				CLInterval cli;
// 				cli.centralmethod = "max bin";
// 				// cli.centralmethod = Form("maximum bin above p=%.2f",pValueThreshold);
// 				cli.central = _pvalues.GetBinCenter(i);
// 				cli.pvalueAtCentral = _pvalues.GetBinContent(i);
// 				_clintervals1sigma.push_back(cli); // push_back stores copies
// 				_clintervals2sigma.push_back(cli);
// 			}
// 		}
// 	}
// }

///
/// Find the interval boundaries corresponding to the central values
/// already saved in _clintervals1sigma or _clintervals2sigma, and to
/// the number of sigmas given. Saves the result into the intevals in
/// _clintervals1sigma or _clintervals2sigma.
/// \param pvalue - pvalue of the intervals to find
/// \param clis - list of confidence intervals holding the central value
///
void CLIntervalMaker::findRawIntervals(float pvalue, vector<CLInterval> &clis) const
{
	for ( int i=0; i<clis.size(); i++ ){
		if ( clis[i].pvalueAtCentral<pvalue ) continue; // skip central values that will not going to be included in an interval at this pvalue
		clis[i].pvalue = pvalue;
		clis[i].minmethod = "bins";
		clis[i].maxmethod = "bins";
		int centralValueBin = valueToBin(clis[i].central);

	  // find lower interval bound
		clis[i].min = _pvalues.GetXaxis()->GetXmin();
    for ( int j=centralValueBin; j>0; j-- ){
    	if ( _pvalues.GetBinContent(j) < pvalue ){
				clis[i].min = _pvalues.GetBinCenter(j);
        break;
      }
   	}

	  // find upper interval bound
		clis[i].max = _pvalues.GetXaxis()->GetXmax();
    for ( int j=centralValueBin; j<_pvalues.GetNbinsX(); j++ ){
    	if ( _pvalues.GetBinContent(j) < pvalue ){
				clis[i].max = _pvalues.GetBinCenter(j);
        break;
      }
   	}

		// check if both boundaries were found
		clis[i].closed = ( clis[i].min != _pvalues.GetXaxis()->GetXmin()
			              && clis[i].max != _pvalues.GetXaxis()->GetXmax() );
	}
}

///
/// Remove bad intervals, where findRawIntervals() couldn't find
/// interval boundaries corresponding to the central value and pvalue
/// given.
///
void CLIntervalMaker::removeBadIntervals()
{
	vector<CLInterval> _clintervals1sigmaTmp;
  vector<CLInterval> _clintervals2sigmaTmp;
	for ( int i=0; i<_clintervals1sigma.size(); i++ ){
		if ( _clintervals1sigma[i].pvalue>0 ) _clintervals1sigmaTmp.push_back(_clintervals1sigma[i]);
	}
	for ( int i=0; i<_clintervals2sigma.size(); i++ ){
		if ( _clintervals2sigma[i].pvalue>0 ) _clintervals2sigmaTmp.push_back(_clintervals2sigma[i]);
	}
	_clintervals1sigma = _clintervals1sigmaTmp;
	_clintervals2sigma = _clintervals2sigmaTmp;
}

///
/// Check if both bins i and i+1 of the _pvalues histogram
/// are on the same side of y.
/// \param i - bin
/// \param y - y value
///
bool CLIntervalMaker::binsOnSameSide(int i, float y) const
{
	return (  (_pvalues.GetBinContent(i)>y && _pvalues.GetBinContent(i+1)>y)
         || (_pvalues.GetBinContent(i)<y && _pvalues.GetBinContent(i+1)<y) );
}

int CLIntervalMaker::checkNeighboringBins(int i, float y) const
{
	// cout << "CLIntervalMaker::checkNeighboringBins() : " << i << " " << y << endl;
	if ( !binsOnSameSide(i, y) ) return i;
	if ( 1 < i+1 && i+1 <= _pvalues.GetNbinsX()-1 && !binsOnSameSide(i+1, y) ) return i+1;
	if ( 1 < i-1 && i-1 <= _pvalues.GetNbinsX()-1 && !binsOnSameSide(i-1, y) ) return i-1;
	// cout << "CLIntervalMaker::checkNeighboringBins() : WARNING : couldn't find better bin: " << i << endl;
	return i;
}

///
/// Find an interpolated x value near a certain bin position of a histogram that is the
/// best estimate for h(x)=y. Interpolates by means of a straight line between two
/// known points.
/// \param h - the histogram to be interpolated
/// \param i - interpolate around this bin. Must be a bin such that i and i+1 are above and below val
/// \param y - the y position we want to find the interpolated x for
/// \param val - Return value: interpolated x position
/// \return true if successful
///
bool CLIntervalMaker::interpolateLine(const TH1F* h, int i, float y, float &val) const
{
	// cout << "CLIntervalMaker::interpolateLine(): i=" << i << " y=" << y << endl;
  if ( !( 1 <= i && i <= h->GetNbinsX()-1 ) ) return false;
	if ( binsOnSameSide(i, y) ) {
		cout << "CLIntervalMaker::interpolateLine() : ERROR : bins i and i+1 on same side of y" << endl;
		return false;
	}
  float p1x = h->GetBinCenter(i);
	float p1y = h->GetBinContent(i);
  float p2x = h->GetBinCenter(i+1);
  float p2y = h->GetBinContent(i+1);
  val = p2x + (y-p2y)/(p1y-p2y)*(p1x-p2x);
	return true;
}

///
/// Improve the intervals through an interpolation with a straight line.
/// \param clis - list of confidence intervals holding the central value and min and max boundaries
///
void CLIntervalMaker::improveIntervalsLine(vector<CLInterval> &clis) const
{
	for ( int i=0; i<clis.size(); i++ ){
		bool wasImproved;
		float newMin, newMax;
		int binMin, binMax;
		// improve lower boundary
		binMin = checkNeighboringBins(valueToBin(clis[i].min), clis[i].pvalue);
		wasImproved = interpolateLine(&_pvalues, binMin, clis[i].pvalue, newMin);
		if ( wasImproved ){
			clis[i].minmethod = "line";
			clis[i].min = newMin;
		}
		// improve upper boundary
		binMax = checkNeighboringBins(valueToBin(clis[i].max), clis[i].pvalue);
		wasImproved = interpolateLine(&_pvalues, binMax, clis[i].pvalue, newMax);
		if ( wasImproved ){
			clis[i].maxmethod = "line";
			clis[i].max = newMax;
		}
	}
}

///
/// Improve the intervals through an fit with a pol2.
/// \param clis - list of confidence intervals holding the central value and min and max boundaries
///
void CLIntervalMaker::improveIntervalsPol2fit(vector<CLInterval> &clis) const
{
	for ( int i=0; i<clis.size(); i++ ){
		bool wasImproved;
		float newMin, newMax, newMinErr, newMaxErr;
		int binMin, binMax;
		// improve lower boundary
		binMin = checkNeighboringBins(valueToBin(clis[i].min), clis[i].pvalue);
		wasImproved = interpolatePol2fit(&_pvalues, binMin, clis[i].pvalue, clis[i].central, false, newMin, newMinErr);
		if ( wasImproved ){
			clis[i].minmethod = "pol2";
			clis[i].min = newMin;
		}
		// improve upper boundary
		binMax = checkNeighboringBins(valueToBin(clis[i].max), clis[i].pvalue);
		wasImproved = interpolatePol2fit(&_pvalues, binMax, clis[i].pvalue, clis[i].central, true, newMax, newMaxErr);
		if ( wasImproved ){
			clis[i].maxmethod = "pol2";
			clis[i].max = newMax;
		}
	}
}

///
/// Solve a quadratic equation by means of a modified pq formula:
/// @f[x^2 + \frac{p_1}{p_2} x + \frac{p_0-y}{p2} = 0@f]
///
float CLIntervalMaker::pq(float p0, float p1, float p2, float y, int whichSol) const
{
  if ( whichSol == 0 ) return -p1/2./p2 + sqrt( sq(p1/2./p2) - (p0-y)/p2 );
  else                 return -p1/2./p2 - sqrt( sq(p1/2./p2) - (p0-y)/p2 );
}

///
/// Find an interpolated x value near a certain bin position of a histogram that is the
/// best estimate for h(x)=y. Interpolates by means of fitting a second grade polynomial
/// to up to five adjacent points. Because that's giving us two solutions, we use the central
/// value and knowledge about if it is supposed to be an upper or lower boundary to pick
/// one.
/// \param h - the histogram to be interpolated
/// \param i - interpolate around this bin. Must be a bin such that i and i+1 are above and below val
/// \param y - the y position we want to find the interpolated x for
/// \param central - Central value of the solution we're trying to get the CL interval for.
/// \param upper - Set to true if we're computing an upper interval boundary.
/// \param val - Return value: interpolated x position
/// \param err - Return value: estimated interpolation error
/// \return true, if inpterpolation was performed, false, if conditions were not met
///
bool CLIntervalMaker::interpolatePol2fit(const TH1F* h, int i, float y, float central, bool upper,
	float &val, float &err) const
{
	// cout << "CLIntervalMaker::interpolatePol2fit(): i=" << i << " y=" << y << " central=" << central << endl;
  if ( !( 2 <= i && i <= h->GetNbinsX()-1 ) ) return false;
	if ( binsOnSameSide(i-1, y) && binsOnSameSide(i, y) ){
		cout << "CLIntervalMaker::interpolatePol2fit() : ERROR : bins i-1, i, and i+1 on same side of y" << endl;
		return false;
	}

  // create a TGraph that we can fit
  TGraph *g = new TGraph(3);
  g->SetPoint(0, h->GetBinCenter(i-1), h->GetBinContent(i-1));
  g->SetPoint(1, h->GetBinCenter(i),   h->GetBinContent(i));
  g->SetPoint(2, h->GetBinCenter(i+1), h->GetBinContent(i+1));

  // see if we can add a 4th and 5th point:
  // add a point to the beginning
  if ( 3 <= i && i <= h->GetNbinsX()-2 ){
	  if ( (h->GetBinContent(i-2) < h->GetBinContent(i-1) && h->GetBinContent(i-1) < h->GetBinContent(i))
	    || (h->GetBinContent(i-2) > h->GetBinContent(i-1) && h->GetBinContent(i-1) > h->GetBinContent(i)) ){
	    TGraph *gNew = new TGraph(g->GetN()+1);
	    gNew->SetPoint(0, h->GetBinCenter(i-2), h->GetBinContent(i-2));
	    Double_t pointx, pointy;
	    for ( int i=0; i<g->GetN(); i++ ){
	      g->GetPoint(i, pointx, pointy);
	      gNew->SetPoint(i+1, pointx, pointy);
	    }
	    delete g;
	    g = gNew;
	  }
	  // add a point to the end
	  if ( (h->GetBinContent(i+2) < h->GetBinContent(i+1) && h->GetBinContent(i+1) < h->GetBinContent(i))
	    || (h->GetBinContent(i+2) > h->GetBinContent(i+1) && h->GetBinContent(i+1) > h->GetBinContent(i)) ){
	    g->Set(g->GetN()+1);
	    g->SetPoint(g->GetN()-1, h->GetBinCenter(i+2), h->GetBinContent(i+2));
	  }
	}

  // debug: show fitted 1-CL histogram
  // if ( y<0.1 )
  // // if ( methodName == TString("Plugin") && y<0.1 )
  // {
  //   // TString debugTitle = methodName + Form(" y=%.2f ",y);
  //   // debugTitle += upper?Form("%f upper",central):Form("%f lower",central);
  // 		TString debugTitle = "honk";
  //   TCanvas *c = newNoWarnTCanvas(getUniqueRootName(), debugTitle);
  //   g->SetMarkerStyle(3);
  //   g->SetHistogram(const_cast<TH1F*>(h));
  //   const_cast<TH1F*>(h)->Draw();
  //   g->Draw("p");
  // }

	// fit
  TF1 *f1 = new TF1("f1", "pol2", h->GetBinCenter(i-2), h->GetBinCenter(i+2));
  g->Fit("f1", "q");    // fit linear to get decent start parameters
  g->Fit("f1", "qf+");  // refit with minuit to get more correct errors (TGraph fit errors bug)
  float p[3], e[3];
  for ( int ii=0; ii<3; ii++ ){
    p[ii] = f1->GetParameter(ii);
    e[ii] = f1->GetParError(ii);
  }

	// get solution by solving the pol2 for x
  float sol0 = pq(p[0], p[1], p[2], y, 0);
  float sol1 = pq(p[0], p[1], p[2], y, 1);

	// decide which of both solutions to use based on the position of
	// the central value
  int useSol = 0;
  if ( (sol0<central && sol1>central) || (sol1<central && sol0>central) ){
    if ( upper ){
      if ( sol0<sol1 ) useSol = 1;
      else             useSol = 0;
    }
    else{
      if ( sol0<sol1 ) useSol = 0;
      else             useSol = 1;
    }
  }
  else{
    if ( fabs(h->GetBinCenter(i)-sol0) < fabs(h->GetBinCenter(i)-sol1) ) useSol = 0;
    else useSol = 1;
  }
  if ( useSol==0 ) val = sol0;
  else             val = sol1;

  // try error propagation: sth is wrong in the formulae
  // float err0 = TMath::Max(sq(val-pq(p[0]+e[0], p[1], p[2], y, useSol)), sq(val-pq(p[0]-e[0], p[1], p[2], y, useSol)));
  // float err1 = TMath::Max(sq(val-pq(p[0], p[1]+e[1], p[2], y, useSol)), sq(val-pq(p[0], p[1]-e[1], p[2], y, useSol)));
  // float err2 = TMath::Max(sq(val-pq(p[0], p[1], p[2]+e[2], y, useSol)), sq(val-pq(p[0], p[1], p[2]-e[2], y, useSol)));
  // err = sqrt(err0+err1+err2);
  // printf("%f %f %f\n", val, pq(p[0]+e[0], p[1], p[2], y, useSol), pq(p[0]-e[0], p[1], p[2], y, useSol));
  // printf("%f %f %f\n", val, pq(p[0], p[1]+e[1], p[2], y, useSol), pq(p[0], p[1]-e[1], p[2], y, useSol));
  // printf("%f %f %f\n", val, pq(p[0], p[1], p[2]+e[2], y, useSol), pq(p[0], p[1], p[2]-e[2], y, useSol));
  err = 0.0;
	// delete g;
  return true;
}


int CLIntervalMaker::valueToBin(float val) const
{
	return _pvalues.GetXaxis()->FindBin(val);
}

float	CLIntervalMaker::binToValue(int bin) const
{
	return _pvalues.GetBinCenter(bin);
}

void CLIntervalMaker::print()
{
	CLIntervalPrinter clp(_arg, "test", "var", "", "n/a");
	clp.setDegrees(false);
	clp.addIntervals(_clintervals1sigma);
	clp.addIntervals(_clintervals2sigma);
	clp.print();
	cout << endl;
}

///
/// Calculate the CL intervals.
///
void CLIntervalMaker::calcCLintervals()
{
	// _clintervals1sigma.clear();
	// _clintervals2sigma.clear();
	// findMaxima(0.04); // ignore maxima under pvalue=0.04
	// print();

	// cout << "findRawIntervals()" << endl;
	findRawIntervals(1.-0.6827, _clintervals1sigma);
	findRawIntervals(1.-0.9545, _clintervals2sigma);
	// print();

	// for ( int i=0; i<_clintervals1sigma.size(); i++ ){
	// 	_clintervals1sigma[i].print();
	// }
	// for ( int i=0; i<_clintervals2sigma.size(); i++ ){
	// 	_clintervals2sigma[i].print();
	// }

	// cout << "removeBadIntervals()" << endl;
	removeBadIntervals();
	// print();

	// cout << "improveIntervalsLine()" << endl;
	improveIntervalsLine(_clintervals1sigma);
	improveIntervalsLine(_clintervals2sigma);
	// print();

	// cout << "improveIntervalsPol2fit()" << endl;
	improveIntervalsPol2fit(_clintervals1sigma);
	improveIntervalsPol2fit(_clintervals2sigma);
	print();
}
