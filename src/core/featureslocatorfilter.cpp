#include "featureslocatorfilter.h"


#include <qgsproject.h>
#include <qgsvectorlayer.h>
#include <qgsmaplayermodel.h>
#include <qgsfeedback.h>


QgsAllLayersFeaturesLocatorFilter::QgsAllLayersFeaturesLocatorFilter( QObject *parent )
  : QgsLocatorFilter( parent )
{
  setUseWithoutPrefix( false );
}

QgsAllLayersFeaturesLocatorFilter *QgsAllLayersFeaturesLocatorFilter::clone() const
{
  return new QgsAllLayersFeaturesLocatorFilter();
}

void QgsAllLayersFeaturesLocatorFilter::prepare( const QString &string, const QgsLocatorContext &context )
{
  if ( string.length() < 3 || context.usingPrefix )
    return;

  const QMap<QString, QgsMapLayer *> layers = QgsProject::instance()->mapLayers();
  for ( auto it = layers.constBegin(); it != layers.constEnd(); ++it )
  {
    QgsVectorLayer *layer = qobject_cast< QgsVectorLayer *>( it.value() );
    if ( !layer || !layer->flags().testFlag( QgsMapLayer::Searchable ) )
      continue;

    QgsExpression expression( layer->displayExpression() );
    QgsExpressionContext context;
    context.appendScopes( QgsExpressionContextUtils::globalProjectLayerScopes( layer ) );
    expression.prepare( &context );

    QgsFeatureRequest req;
    req.setSubsetOfAttributes( expression.referencedAttributeIndexes( layer->fields() ).toList() );
    if ( !expression.needsGeometry() )
      req.setFlags( QgsFeatureRequest::NoGeometry );
    req.setFilterExpression( QStringLiteral( "%1 ILIKE '%%2%'" )
                             .arg( layer->displayExpression() )
                             .arg( string ) );
    req.setLimit( 30 );

    PreparedLayer preparedLayer;
    preparedLayer.expression = expression;
    preparedLayer.context = context;
    preparedLayer.layerId = layer->id();
    preparedLayer.layerName = layer->name();
    preparedLayer.iterator =  layer->getFeatures( req );
    preparedLayer.layerIcon = QgsMapLayerModel::iconForLayer( layer );

    mPreparedLayers.append( preparedLayer );
  }
}

void QgsAllLayersFeaturesLocatorFilter::fetchResults( const QString &string, const QgsLocatorContext &, QgsFeedback *feedback )
{
  int foundInCurrentLayer;
  int foundInTotal = 0;
  QgsFeature f;

  // we cannot used const loop since iterator::nextFeature is not const
  for ( PreparedLayer preparedLayer : mPreparedLayers )
  {
    foundInCurrentLayer = 0;
    while ( preparedLayer.iterator.nextFeature( f ) )
    {
      if ( feedback->isCanceled() )
        return;

      QgsLocatorResult result;
      result.group = preparedLayer.layerName;

      preparedLayer.context.setFeature( f );

      result.displayString = preparedLayer.expression.evaluate( &( preparedLayer.context ) ).toString();

      result.userData = QVariantList() << f.id() << preparedLayer.layerId;
      result.icon = preparedLayer.layerIcon;
      result.score = static_cast< double >( string.length() ) / result.displayString.size();
      emit resultFetched( result );

      foundInCurrentLayer++;
      foundInTotal++;
      if ( foundInCurrentLayer >= mMaxResultsPerLayer )
        break;
    }
    if ( foundInTotal >= mMaxTotalResults )
      break;
  }
}

void QgsAllLayersFeaturesLocatorFilter::triggerResult( const QgsLocatorResult &result )
{
  QVariantList dataList = result.userData.toList();
  QgsFeatureId id = dataList.at( 0 ).toLongLong();
  QString layerId = dataList.at( 1 ).toString();
  QgsVectorLayer *layer = qobject_cast< QgsVectorLayer *>( QgsProject::instance()->mapLayer( layerId ) );
  if ( !layer )
    return;

  //QgisApp::instance()->mapCanvas()->zoomToFeatureIds( layer, QgsFeatureIds() << id );
}
