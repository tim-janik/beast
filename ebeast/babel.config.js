module.exports = (api) => {
  api.cache.never();

  const presets = [
    [ '@babel/preset-env', {
      targets: {
	"browsers": [
	  "ff 68",
	  "chrome 76",
	], },
    } ],
  ];

  const plugins = [
    '@babel/plugin-proposal-class-properties',
    '@babel/plugin-proposal-export-default-from',
    '@babel/plugin-proposal-export-namespace-from',
    '@babel/plugin-proposal-nullish-coalescing-operator',
    '@babel/plugin-proposal-numeric-separator',
    '@babel/plugin-proposal-object-rest-spread',
    '@babel/plugin-proposal-optional-chaining',
  ];

  return {
    presets,
    plugins,
  };
};
