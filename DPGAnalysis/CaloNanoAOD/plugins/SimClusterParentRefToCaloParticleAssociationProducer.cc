// Builds an edm::Association<CaloParticleCollection> for a SimCluster collection made by
// CaloTruthAccumulator (e.g. mix:MergedCaloTruthBoundaryTrackSimCluster), using the parent
// RefVector the accumulator stores under the same label. Entry i of that RefVector points into
// mix:MergedCaloTruthCaloParticle, which is 1:1 with the CaloParticle collection, so its key is
// the CaloParticle index of SimCluster i.
//
// The output can be fed to SimClusterToCaloParticleIndexTableProducer, like the legacy
// SimClusterToCaloParticleAssociationProducer (which only works for mix:MergedCaloTruth,
// because it relies on CaloParticle::simClusters()).

#include "DataFormats/Common/interface/Association.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "SimDataFormats/CaloAnalysis/interface/CaloParticle.h"
#include "SimDataFormats/CaloAnalysis/interface/CaloParticleFwd.h"
#include "SimDataFormats/CaloAnalysis/interface/SimCluster.h"
#include "SimDataFormats/CaloAnalysis/interface/SimClusterFwd.h"

#include <memory>
#include <vector>

class SimClusterParentRefToCaloParticleAssociationProducer : public edm::global::EDProducer<> {
public:
  explicit SimClusterParentRefToCaloParticleAssociationProducer(const edm::ParameterSet& pset)
      : simClusterToken_(consumes<SimClusterCollection>(pset.getParameter<edm::InputTag>("simClusters"))),
        parentRefsToken_(consumes<SimClusterRefVector>(pset.getParameter<edm::InputTag>("parentRefs"))),
        caloParticleToken_(consumes<CaloParticleCollection>(pset.getParameter<edm::InputTag>("caloParticles"))) {
    produces<edm::Association<CaloParticleCollection>>();
  }

  void produce(edm::StreamID, edm::Event& iEvent, const edm::EventSetup&) const override {
    auto simClusters = iEvent.getHandle(simClusterToken_);
    auto const& parentRefs = iEvent.get(parentRefsToken_);
    auto caloParticles = iEvent.getHandle(caloParticleToken_);

    if (parentRefs.size() != simClusters->size())
      throw cms::Exception("SizeMismatch") << "parentRefs has " << parentRefs.size() << " entries but simClusters has "
                                           << simClusters->size();

    std::vector<int> cpIndices;
    cpIndices.reserve(parentRefs.size());
    for (auto const& ref : parentRefs) {
      int idx = ref.isNonnull() ? static_cast<int>(ref.key()) : -1;
      if (idx >= static_cast<int>(caloParticles->size()))
        throw cms::Exception("IndexOutOfRange")
            << "parent index " << idx << " but only " << caloParticles->size() << " CaloParticles";
      cpIndices.push_back(idx);
    }

    auto assoc = std::make_unique<edm::Association<CaloParticleCollection>>(caloParticles);
    edm::Association<CaloParticleCollection>::Filler filler(*assoc);
    filler.insert(simClusters, cpIndices.begin(), cpIndices.end());
    filler.fill();
    iEvent.put(std::move(assoc));
  }

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("simClusters", edm::InputTag("mix", "MergedCaloTruthBoundaryTrackSimCluster"));
    desc.add<edm::InputTag>("parentRefs", edm::InputTag("mix", "MergedCaloTruthBoundaryTrackSimCluster"));
    desc.add<edm::InputTag>("caloParticles", edm::InputTag("mix", "MergedCaloTruth"));
    descriptions.addWithDefaultLabel(desc);
  }

private:
  const edm::EDGetTokenT<SimClusterCollection> simClusterToken_;
  const edm::EDGetTokenT<SimClusterRefVector> parentRefsToken_;
  const edm::EDGetTokenT<CaloParticleCollection> caloParticleToken_;
};

DEFINE_FWK_MODULE(SimClusterParentRefToCaloParticleAssociationProducer);
